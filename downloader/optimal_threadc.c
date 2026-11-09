
#define _GNU_SOURCE
#define _POSIX_C_SOURCE 199309L

#include <time.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>

#include "impheads.h"
#include "tls.h"
#include "optimal_threadc.h"
#include "parser.h"

#define MIN_CHUNK_SIZE (8 * 1024 * 1024)
#define MAX_WORKERS 4


// Factor in file size, response time, bandwidth and hardware threads.

int optimalnum(double response_time, char *filesize, char *max_bandwidth)
{
    if (filesize == NULL || max_bandwidth == NULL || response_time < 0)
        return -1;

    const char *parsed_size_str = filesize;

    char *endptr;
    errno = 0;

    int64_t file_size = strtoll(parsed_size_str, &endptr, 10);

    if (errno == ERANGE)
    {
        fprintf(stderr, "Filesize exceeds system limit.\n");
        return -1;
    }

    if (endptr == parsed_size_str || *endptr != '\0' || file_size < 0)
    {
        fprintf(stderr, "Invalid filesize.\n");
        return -1;
    }

    if (file_size == 0)
        return -1;

    // Get hardware threads.
    long threads = sysconf(_SC_NPROCESSORS_ONLN);

    if (threads < 1)
        threads = 1;


    // Parse maximum bandwidth.
    errno = 0;

    long bandwidthf = strtol(max_bandwidth, &endptr, 10);

    if (errno == ERANGE || endptr == max_bandwidth ||
        *endptr != '\0' || bandwidthf <= 0)
    {
        printf("Invalid bandwidth\n");
        return -1;
    }
bandwidthf = bandwidthf * 1000000L;
    // Estimate download time using maximum bandwidth.
    double estimated_time = ((double)file_size * 8.0) / bandwidthf;

    // If downloading is already quick, use one worker.
    if (response_time > 0 && estimated_time <= response_time)
        return 1;


    // File is large enough for multiple chunks.
    long size_workers = file_size / MIN_CHUNK_SIZE;

    if (size_workers < 2)
        return 1;


    // Hardware-based worker limit.
    long max_workers = threads * 2;

    if (max_workers > MAX_WORKERS)
        max_workers = MAX_WORKERS;


    // Select workers based on file size.
    if (size_workers > max_workers)
        size_workers = max_workers;
printf("Response time: %f seconds\n", response_time);
printf("File size: %ld bytes\n", file_size);
printf("Maximum bandwidth: %ld bps\n", bandwidthf);

    return (int)size_workers;
}


// Server response time.

double transfer_time(struct timespec *start, struct timespec *end)
{
    double seconds = (double)(end->tv_sec - start->tv_sec);

    double nanoseconds =
        (double)(end->tv_nsec - start->tv_nsec) / 1000000000.0;

    return seconds + nanoseconds;
}
