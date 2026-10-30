#ifndef __CONNECTIONS_H_
#define __CONNECTIONS_H_
typedef struct addrinfo as;
typedef struct sockaddr_in net;
int createsocket(struct addrinfo *res, char *ip, size_t ip_len);

#endif

