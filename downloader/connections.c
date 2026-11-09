#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <openssl/x509_vfy.h>
#include <errno.h>
#include "connections.h"
#include "parser.h"
typedef struct addrinfo as;
typedef struct sockaddr_in net;
#define INITIAL_CAPACITY 16384
#define MAX_CAPACITY 65536

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


char *notls(struct parser *url,struct timespec *start_time,struct timespec *end_time,int sockfd)
{
    char *headreq = "HEAD %s HTTP/1.1\r\n"
                    "Host: %s\r\n"
                    "Connection: close\r\n\r\n";

    int needed = snprintf(NULL, 0, headreq, url->path, url->hostname);

    if (needed < 0)
        return NULL;

    size_t buffer_size = (size_t)needed + 1;
    char buffer[buffer_size];

    snprintf(buffer, buffer_size, headreq, url->path, url->hostname);

    size_t actual_written = 0;
    size_t request_len = strlen(buffer);

    while (actual_written < request_len)
    {
        ssize_t bytes_written = write(sockfd,buffer + actual_written,request_len - actual_written);

        if (bytes_written < 0)
        {
            if (errno == EINTR)
                continue;

            perror("write");
            return NULL;
        }

        if (bytes_written == 0)
            return NULL;

        actual_written += (size_t)bytes_written;
    }

    size_t capacity = INITIAL_CAPACITY;

    char *reciev = malloc(capacity + 1);

    if (reciev == NULL)
    {
        perror("malloc");
        return NULL;
    }

    size_t total_read = 0;

    // Start measuring HTTP response time.
    if (clock_gettime(CLOCK_MONOTONIC, start_time) != 0)
    {
        perror("clock_gettime");
        free(reciev);
        return NULL;
    }

    while (1)
    {
        if (total_read == capacity)
        {
            if (capacity >= MAX_CAPACITY)
            {
                fprintf(stderr, "Response headers too large\n");
                free(reciev);
                return NULL;
            }

            size_t new_capacity = capacity * 2;

            if (new_capacity > MAX_CAPACITY)
            {

             
            new_capacity = MAX_CAPACITY;
            }
            char *temp = realloc(reciev, new_capacity + 1);

            if (temp == NULL)
            {
                perror("realloc");
                free(reciev);
                return NULL;
            }

            reciev = temp;
            capacity = new_capacity;
        }

        ssize_t bytes_read = read(sockfd, reciev + total_read, capacity - total_read);

        if (bytes_read < 0)
        {
            if (errno == EINTR)
                continue;

            perror("read");
            free(reciev);
            return NULL;
        }

        if (bytes_read == 0)
        {
            fprintf(stderr, "Connection closed before headers completed\n");
            free(reciev);
            return NULL;
        }

        total_read += (size_t)bytes_read;
        reciev[total_read] = '\0';

        if (strstr(reciev, "\r\n\r\n") != NULL)
        {
            if (clock_gettime(CLOCK_MONOTONIC, end_time) != 0)
            {
                perror("clock_gettime");
                free(reciev);
                return NULL;
            }

            return reciev;
        }
    }
}