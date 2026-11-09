#ifndef __CONNECTIONS_H_
#define __CONNECTIONS_H_
typedef struct addrinfo as;
typedef struct sockaddr_in net;
struct parser;
struct timespec;
int createsocket(struct addrinfo *res, char *ip, size_t ip_len);
char *notls(struct parser *url,struct timespec *start_time,struct timespec *end_time,int sockfd);

#endif

