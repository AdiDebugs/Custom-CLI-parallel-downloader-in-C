
#ifndef __OPTIMAL_THREADC_H_
#define __OPTIMAL_THREADC_H_


double transfer_time(struct timespec *start, struct timespec *end);
int optimalnum(double response_time, char *filesize, char *max_bandwidth);

#endif