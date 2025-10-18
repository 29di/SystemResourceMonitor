#ifndef MONITOR_H
#define MONITOR_H

#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>
#include <mqueue.h>

// Configuration thresholds and sampling interval
typedef struct {
    double cpu_alert_threshold;     // percent
    double mem_alert_threshold;     // percent
    unsigned int sample_interval_ms; // sampling interval for producers
    unsigned int summary_interval_s; // how often to emit IPC summary
} monitor_config_t;

// Metric kinds
typedef enum {
    METRIC_CPU,
    METRIC_MEM,
    METRIC_DISK,
    METRIC_NET,
    METRIC_SUMMARY
} metric_kind_t;

typedef struct {
    metric_kind_t kind;
    double v1; // usage percent or rate1
    double v2; // rate2 or extra
    uint64_t ts_ms; // epoch milliseconds
} metric_t;

// Simple bounded circular buffer for producer-consumer
#define QUEUE_CAP 256
typedef struct {
    metric_t buf[QUEUE_CAP];
    int head; // next write
    int tail; // next read
    int size; // current items
    pthread_mutex_t mtx;
    pthread_cond_t not_empty;
    pthread_cond_t not_full;
} metric_queue_t;

typedef struct {
    volatile bool running;
    monitor_config_t cfg;
    metric_queue_t queue;
    mqd_t mq;              // POSIX message queue for IPC summaries
    char mq_name[64];      // e.g., "/sysmon_queue"
} monitor_ctx_t;

// Queue API
void mq_init(metric_queue_t *q);
void mq_destroy(metric_queue_t *q);
bool mq_push(metric_queue_t *q, const metric_t *m);
bool mq_pop(metric_queue_t *q, metric_t *out);

// Monitor lifecycle
int monitor_run(monitor_ctx_t *ctx);

// Utility
uint64_t now_ms(void);

#endif // MONITOR_H
