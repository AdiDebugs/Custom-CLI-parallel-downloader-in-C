
#ifndef __OPTIMAL_THREADC_H_
#define __OPTIMAL_THREADC_H_

typedef struct threadinfo
{
    pthread_t thread;

    SSL_CTX *ctx;
    int output_fd;
    const struct parser *url;

    off_t start_offset;
    off_t end_offset;

    int result;
}threadinfo;

double transfer_time(struct timespec *start, struct timespec *end);
int optimalnum(double response_time, char *filesize, char *max_bandwidth);

#endif