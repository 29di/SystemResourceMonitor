#define _GNU_SOURCE
#include "monitor.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/stat.h>

// Windows note: This program targets Linux systems with /proc and POSIX mqueue.
// On Windows, run via WSL or a Linux environment.

static volatile sig_atomic_t g_stop = 0;

static void on_sigint(int sig) {
    (void)sig;
    g_stop = 1;
}

uint64_t now_ms(void) {
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    return (uint64_t)ts.tv_sec * 1000ULL + ts.tv_nsec / 1000000ULL;
}

void mq_init(metric_queue_t *q) {
    q->head = q->tail = q->size = 0;
    pthread_mutex_init(&q->mtx, NULL);
    pthread_cond_init(&q->not_empty, NULL);
    pthread_cond_init(&q->not_full, NULL);
}

void mq_destroy(metric_queue_t *q) {
    pthread_mutex_destroy(&q->mtx);
    pthread_cond_destroy(&q->not_empty);
    pthread_cond_destroy(&q->not_full);
}

bool mq_push(metric_queue_t *q, const metric_t *m) {
    pthread_mutex_lock(&q->mtx);
    while (q->size == QUEUE_CAP && !g_stop) {
        pthread_cond_wait(&q->not_full, &q->mtx);
    }
    if (g_stop) { pthread_mutex_unlock(&q->mtx); return false; }
    q->buf[q->head] = *m;
    q->head = (q->head + 1) % QUEUE_CAP;
    q->size++;
    pthread_cond_signal(&q->not_empty);
    pthread_mutex_unlock(&q->mtx);
    return true;
}

bool mq_pop(metric_queue_t *q, metric_t *out) {
    pthread_mutex_lock(&q->mtx);
    while (q->size == 0 && !g_stop) {
        pthread_cond_wait(&q->not_empty, &q->mtx);
    }
    if (q->size == 0) { pthread_mutex_unlock(&q->mtx); return false; }
    *out = q->buf[q->tail];
    q->tail = (q->tail + 1) % QUEUE_CAP;
    q->size--;
    pthread_cond_signal(&q->not_full);
    pthread_mutex_unlock(&q->mtx);
    return true;
}

// Helper: read a full file into buffer
static int read_file(const char *path, char *buf, size_t cap) {
    FILE *f = fopen(path, "r");
    if (!f) return -1;
    size_t n = fread(buf, 1, cap - 1, f);
    buf[n] = '\0';
    fclose(f);
    return (int)n;
}

// CPU usage based on /proc/stat delta
typedef struct { unsigned long long user,nice,system,idle,iowait,irq,softirq,steal,guest,guest_nice; } cpu_times_t;

static int read_cpu_times(cpu_times_t *t) {
    char line[512];
    FILE *f = fopen("/proc/stat", "r");
    if (!f) return -1;
    if (!fgets(line, sizeof(line), f)) { fclose(f); return -1; }
    fclose(f);
    // cpu  3357 0 4313 1362393 0 0 0 0 0 0
    int n = sscanf(line, "cpu %llu %llu %llu %llu %llu %llu %llu %llu %llu %llu",
                   &t->user, &t->nice, &t->system, &t->idle, &t->iowait,
                   &t->irq, &t->softirq, &t->steal, &t->guest, &t->guest_nice);
    return (n >= 4) ? 0 : -1;
}

static double cpu_usage_percent(void) {
    cpu_times_t a,b;
    if (read_cpu_times(&a) != 0) return -1.0;
    usleep(200000); // 200ms sample
    if (read_cpu_times(&b) != 0) return -1.0;
    unsigned long long idle_a = a.idle + a.iowait;
    unsigned long long idle_b = b.idle + b.iowait;
    unsigned long long non_a = a.user + a.nice + a.system + a.irq + a.softirq + a.steal;
    unsigned long long non_b = b.user + b.nice + b.system + b.irq + b.softirq + b.steal;
    unsigned long long idle_delta = idle_b - idle_a;
    unsigned long long non_delta = non_b - non_a;
    unsigned long long total = idle_delta + non_delta;
    if (total == 0) return 0.0;
    return 100.0 * ((double)non_delta / (double)total);
}

static double mem_usage_percent(void) {
    FILE *f = fopen("/proc/meminfo", "r");
    if (!f) return -1.0;
    char key[64]; unsigned long long val; char unit[16];
    unsigned long long memTotal=0, memAvailable=0;
    while (fscanf(f, "%63s %llu %15s", key, &val, unit) == 3) {
        if (strcmp(key, "MemTotal:") == 0) memTotal = val;
        else if (strcmp(key, "MemAvailable:") == 0) { memAvailable = val; break; }
    }
    fclose(f);
    if (memTotal == 0) return -1.0;
    double used = (double)(memTotal - memAvailable);
    return 100.0 * used / (double)memTotal;
}

static int parse_disk_io(unsigned long long *reads, unsigned long long *writes) {
    // Sum across all disks from /proc/diskstats
    FILE *f = fopen("/proc/diskstats", "r");
    if (!f) return -1;
    unsigned long long r=0,w=0; char line[512];
    while (fgets(line, sizeof(line), f)) {
        // fields: major minor name reads ... writes ...
        // We approximate by picking sectors read (field 6) and written (field 10)
        unsigned int major, minor; char name[64];
        unsigned long long rd_ios, rd_merges, rd_sectors, rd_ticks;
        unsigned long long wr_ios, wr_merges, wr_sectors, wr_ticks;
        int n = sscanf(line, "%u %u %63s %llu %llu %llu %llu %llu %llu %llu %llu",
                       &major, &minor, name,
                       &rd_ios, &rd_merges, &rd_sectors, &rd_ticks,
                       &wr_ios, &wr_merges, &wr_sectors, &wr_ticks);
        if (n >= 11) {
            r += rd_sectors; w += wr_sectors;
        }
    }
    fclose(f);
    *reads = r; *writes = w;
    return 0;
}

static int parse_net_bytes(unsigned long long *rx, unsigned long long *tx) {
    FILE *f = fopen("/proc/net/dev", "r");
    if (!f) return -1;
    char line[512]; int lineNo=0;
    unsigned long long r=0,t=0;
    while (fgets(line, sizeof(line), f)) {
        lineNo++;
        if (lineNo <= 2) continue; // headers
        // iface: rxBytes ... txBytes ...
        char iface[64]; unsigned long long rbytes, others[7], tbytes, others2[7];
        int n = sscanf(line, " %63[^:]: %llu %llu %llu %llu %llu %llu %llu %llu %llu %llu %llu %llu %llu %llu",
                       iface,
                       &rbytes, &others[0], &others[1], &others[2], &others[3], &others[4], &others[5], &others[6],
                       &tbytes, &others2[0], &others2[1], &others2[2], &others2[3], &others2[4], &others2[5]);
        if (n >= 10) { r += rbytes; t += tbytes; }
    }
    fclose(f);
    *rx = r; *tx = t;
    return 0;
}

typedef struct { monitor_ctx_t *ctx; } thread_arg_t;

static void *cpu_thread(void *arg) {
    thread_arg_t *a = (thread_arg_t*)arg;
    while (!g_stop && a->ctx->running) {
        metric_t m = { .kind = METRIC_CPU, .v1 = cpu_usage_percent(), .v2 = 0, .ts_ms = now_ms() };
        mq_push(&a->ctx->queue, &m);
        usleep(a->ctx->cfg.sample_interval_ms * 1000);
    }
    return NULL;
}

static void *mem_thread(void *arg) {
    thread_arg_t *a = (thread_arg_t*)arg;
    while (!g_stop && a->ctx->running) {
        metric_t m = { .kind = METRIC_MEM, .v1 = mem_usage_percent(), .v2 = 0, .ts_ms = now_ms() };
        mq_push(&a->ctx->queue, &m);
        usleep(a->ctx->cfg.sample_interval_ms * 1000);
    }
    return NULL;
}

static void *disk_thread(void *arg) {
    thread_arg_t *a = (thread_arg_t*)arg;
    unsigned long long r0=0,w0=0; parse_disk_io(&r0,&w0);
    while (!g_stop && a->ctx->running) {
        usleep(a->ctx->cfg.sample_interval_ms * 1000);
        unsigned long long r1=0,w1=0; parse_disk_io(&r1,&w1);
        double dr = (double)(r1 - r0);
        double dw = (double)(w1 - w0);
        r0=r1; w0=w1;
        metric_t m = { .kind = METRIC_DISK, .v1 = dr, .v2 = dw, .ts_ms = now_ms() };
        mq_push(&a->ctx->queue, &m);
    }
    return NULL;
}

static void *net_thread(void *arg) {
    thread_arg_t *a = (thread_arg_t*)arg;
    unsigned long long rx0=0,tx0=0; parse_net_bytes(&rx0,&tx0);
    while (!g_stop && a->ctx->running) {
        usleep(a->ctx->cfg.sample_interval_ms * 1000);
        unsigned long long rx1=0,tx1=0; parse_net_bytes(&rx1,&tx1);
        double drx = (double)(rx1 - rx0);
        double dtx = (double)(tx1 - tx0);
        rx0=rx1; tx0=tx1;
        metric_t m = { .kind = METRIC_NET, .v1 = drx, .v2 = dtx, .ts_ms = now_ms() };
        mq_push(&a->ctx->queue, &m);
    }
    return NULL;
}

static void *logger_thread(void *arg) {
    thread_arg_t *a = (thread_arg_t*)arg;
    const char *log_path = "data/logs/resource_log.txt";
    FILE *log = fopen(log_path, "a");
    if (!log) { perror("fopen log"); return NULL; }

    uint64_t last_summary = now_ms();
    double last_cpu = 0.0, last_mem = 0.0;
    while (!g_stop && a->ctx->running) {
        metric_t m;
        if (!mq_pop(&a->ctx->queue, &m)) continue;
        // Log line
        switch (m.kind) {
            case METRIC_CPU:
                last_cpu = m.v1;
                fprintf(log, "%llu,CPU,%.2f\n", (unsigned long long)m.ts_ms, m.v1);
                if (m.v1 >= a->ctx->cfg.cpu_alert_threshold)
                    fprintf(log, "%llu,ALERT,CPU_HIGH,%.2f\n", (unsigned long long)m.ts_ms, m.v1);
                break;
            case METRIC_MEM:
                last_mem = m.v1;
                fprintf(log, "%llu,MEM,%.2f\n", (unsigned long long)m.ts_ms, m.v1);
                if (m.v1 >= a->ctx->cfg.mem_alert_threshold)
                    fprintf(log, "%llu,ALERT,MEM_HIGH,%.2f\n", (unsigned long long)m.ts_ms, m.v1);
                break;
            case METRIC_DISK:
                fprintf(log, "%llu,DISK,%.0f,%.0f\n", (unsigned long long)m.ts_ms, m.v1, m.v2);
                break;
            case METRIC_NET:
                fprintf(log, "%llu,NET,%.0f,%.0f\n", (unsigned long long)m.ts_ms, m.v1, m.v2);
                break;
            default: break;
        }
        fflush(log);

        // Periodic IPC summary
        if (now_ms() - last_summary >= a->ctx->cfg.summary_interval_s * 1000ULL) {
            last_summary = now_ms();
            char msg[128];
            snprintf(msg, sizeof(msg), "CPU=%.1f%% MEM=%.1f%%", last_cpu, last_mem);
            if (a->ctx->mq != (mqd_t)-1) {
                mq_send(a->ctx->mq, msg, strlen(msg)+1, 0);
            }
        }
    }
    fclose(log);
    return NULL;
}

static int open_ipc_queue(monitor_ctx_t *ctx) {
    struct mq_attr attr = {0};
    attr.mq_maxmsg = 10; attr.mq_msgsize = 128;
    mqd_t q = mq_open(ctx->mq_name, O_CREAT | O_WRONLY, 0644, &attr);
    if (q == (mqd_t)-1) {
        perror("mq_open");
        return -1;
    }
    ctx->mq = q;
    return 0;
}

int monitor_run(monitor_ctx_t *ctx) {
    signal(SIGINT, on_sigint);
    ctx->running = true;
    mq_init(&ctx->queue);
    if (open_ipc_queue(ctx) != 0) {
        fprintf(stderr, "IPC queue disabled.\n");
        ctx->mq = (mqd_t)-1;
    }

    pthread_t t_cpu, t_mem, t_disk, t_net, t_log;
    thread_arg_t arg = { .ctx = ctx };
    pthread_create(&t_cpu, NULL, cpu_thread, &arg);
    pthread_create(&t_mem, NULL, mem_thread, &arg);
    pthread_create(&t_disk, NULL, disk_thread, &arg);
    pthread_create(&t_net, NULL, net_thread, &arg);
    pthread_create(&t_log, NULL, logger_thread, &arg);

    // Write PID file for control by main menu
    FILE *pf = fopen("data/monitor.pid", "w");
    if (pf) { fprintf(pf, "%d\n", getpid()); fclose(pf); }

    // Wait until Ctrl+C
    while (!g_stop) {
        sleep(1);
    }
    ctx->running = false;
    // Wake up any waiters
    pthread_mutex_lock(&ctx->queue.mtx);
    pthread_cond_broadcast(&ctx->queue.not_empty);
    pthread_cond_broadcast(&ctx->queue.not_full);
    pthread_mutex_unlock(&ctx->queue.mtx);

    pthread_join(t_cpu, NULL);
    pthread_join(t_mem, NULL);
    pthread_join(t_disk, NULL);
    pthread_join(t_net, NULL);
    pthread_join(t_log, NULL);

    mq_destroy(&ctx->queue);
    if (ctx->mq != (mqd_t)-1) {
        mq_close(ctx->mq);
        mq_unlink(ctx->mq_name);
    }
    unlink("data/monitor.pid");
    return 0;
}

// Main entry for standalone monitor
int main(int argc, char **argv) {
    (void)argc; (void)argv;
    monitor_ctx_t ctx = {0};
    ctx.cfg.cpu_alert_threshold = 85.0;
    ctx.cfg.mem_alert_threshold = 85.0;
    ctx.cfg.sample_interval_ms = 500;
    ctx.cfg.summary_interval_s = 3;
    snprintf(ctx.mq_name, sizeof(ctx.mq_name), "/sysmon_queue");

    // Ensure log directory exists
    mkdir("data", 0755);
    mkdir("data/logs", 0755);

    printf("Resource Monitor started. Press Ctrl+C to stop.\n");
    printf("Logging to data/logs/resource_log.txt\n");
    printf("Sending summaries to POSIX mq %s (if available)\n", ctx.mq_name);
    return monitor_run(&ctx);
}
