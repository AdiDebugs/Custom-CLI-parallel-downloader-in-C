#define _GNU_SOURCE
#include "parser.h"
#include "impheads.h"
#include "tls.h"
#include "connections.h"
#include "optimal_threadc.h"
typedef struct addrinfo as;
typedef struct sockaddr_in net;

// get url
int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        printf("enter the program name then the domain name followed by ur maximum bandwidth (the network plan u have in Mb/s eg- 300)!\n");
        return 1;
    }

    struct parser url = {0};
    struct http_parser httpresponse = {0};
    struct timespec start_time = {0};
    struct timespec end_time = {0};

    SSL_CTX *s = NULL;
    SSL *tlsc = NULL;

    struct addrinfo hints, *res = NULL;

    int status;
    int sockfd = -1;
    int exit_status = 1;

    char ip[INET_ADDRSTRLEN];
    char *serverresp = NULL;

    // parsing the url
    url = parse(argv[1]);

    if (url.success != 1)
    {
        printf("URL parsing failed!\n");
        goto cleanup;
    }

    if (iscorrect(&url) == 1)
    {
        printf("invalid url!\n");
        goto cleanup;
    }

    // context if https
    if (strcmp(url.protocol, "https") == 0)
    {
        s = sslcont();

        if (s == NULL)
        {
            goto cleanup;
        }
    }

    // zero the structs
    memset(&hints, 0, sizeof(hints));

    // for ipv4 and tcp
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    // dns resolution
    if ((status = getaddrinfo(url.hostname, url.port, &hints, &res)) != 0)
    {
        fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status));
        goto cleanup;
    }

    // try to make a socket
    sockfd = createsocket(res, ip, sizeof(ip));

    freeaddrinfo(res);
    res = NULL;

    if (sockfd == -1)
    {
        printf("couldnt create socket\n");
        goto cleanup;
    }

    // int threads = optimal_number()

    // wrap the existing socket and send the request based on protocol
    if (strcmp(url.protocol, "https") == 0)
    {
        tlsc = createtls(s, sockfd, url.hostname);

        if (tlsc == NULL)
        {
            fprintf(stderr, "TLS initialization failed\n");
            goto cleanup;
        }

        serverresp = tlshttpresp(tlsc, &url, &start_time, &end_time);

        if (serverresp == NULL)
        {
            printf("failed to recieve server response\n");
            goto cleanup;
        }

        if (httpparse(&httpresponse, serverresp) == -1)
        {
            printf("Error in response recieved from the server\n");
            goto cleanup;
        }
       if(httpresponse.ranged_req == 1)
       {
         double resptime = transfer_time(&start_time, &end_time);
         int nthreads = optimalnum(resptime, httpresponse.content_len, argv[2]);
         printf("number of optimal threads is: %d\n", nthreads);
         //implement multithreading
       }


    }
    else
    {
        // todo
    }

    printf("hostname: %s\nport: %s\npath: %s\n", url.hostname, url.port, url.path);
    printf("range supported? %d\ncontent length: %s \nstatus code: %s\nredirect location(if any) %s\n", httpresponse.ranged_req, httpresponse.content_len, httpresponse.status_code, httpresponse.location);
    exit_status = 0;

cleanup:

    if (res != NULL)
        freeaddrinfo(res);

    free(serverresp);

    free(httpresponse.content_len);
    free(httpresponse.status_code);
    if(httpresponse.location != NULL)
    {
    free(httpresponse.location);
    }

    if (tlsc != NULL)
        SSL_free(tlsc);

    if (sockfd != -1)
        close(sockfd);

    if (s != NULL)
        SSL_CTX_free(s);
free(url.path);
free(url.hostname);
    return exit_status;
}



