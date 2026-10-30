#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include "connections.h"
typedef struct addrinfo as;
typedef struct sockaddr_in net;


int createsocket(struct addrinfo *res, char *ip, size_t ip_len)
{
    struct addrinfo *p = res;
    int sockfd = -1;

    // walking down the linked list
    while (p != NULL)
    {

        net *ipv4 = (net *) p->ai_addr;
        void *addr = &(ipv4->sin_addr);

        inet_ntop(p->ai_family, addr, ip, ip_len);

        sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);

        if (sockfd == -1)
        {
            perror("socket creation failed");
            p = p->ai_next;
            continue;
        }
        else
        {
            // if socket formation is successful then connect
            int con = connect(sockfd, p->ai_addr, p->ai_addrlen);
            if (con == -1)
            {
                perror("connect failed");
                close(sockfd);
                sockfd = -1;    // don't accidentally return a closed socket
                p = p->ai_next;
                continue;
            }

            // connected
            break;
        }
    }

    return sockfd;
}
