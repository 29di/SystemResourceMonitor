#include <mqueue.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

// Simple consumer that reads summaries from the POSIX MQ used by the monitor
int main(int argc, char **argv) {
    const char *name = (argc > 1) ? argv[1] : "/sysmon_queue";
    mqd_t q = mq_open(name, O_RDONLY);
    if (q == (mqd_t)-1) {
        perror("mq_open");
        fprintf(stderr, "Ensure the monitor is running and created %s\n", name);
        return 1;
    }
    struct mq_attr attr;
    if (mq_getattr(q, &attr) == -1) { perror("mq_getattr"); mq_close(q); return 1; }
    char *buf = malloc(attr.mq_msgsize);
    if (!buf) { perror("malloc"); mq_close(q); return 1; }
    printf("Listening on MQ %s (Ctrl+C to stop)\n", name);
    while (1) {
        ssize_t n = mq_receive(q, buf, attr.mq_msgsize, NULL);
        if (n >= 0) {
            printf("[Summary] %s\n", buf);
            fflush(stdout);
        } else {
            perror("mq_receive");
            break;
        }
    }
    free(buf);
    mq_close(q);
    return 0;
}
