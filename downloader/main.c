#define _GNU_SOURCE
#include "url_parser.h"
#include "impheads.h"
#include "tls.h"
#include "connections.h"
typedef struct addrinfo as;
typedef struct sockaddr_in net;

// get url
int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        printf("enter the program name then the domain name!\n");
        return 1;
    }
    struct parser url;

    // initialize structs
    as hints, *res;
    int status;
    char ip[INET_ADDRSTRLEN];
    // parsing the url
    url = parse(argv[1]);

    if (url.success != 1)
    {
        printf("URL parsing failed!\n");
        return 1;
    }

    if (iscorrect(&url) == 1)
    {
        printf("invalid url!\n");
        return 1;
    }
    //context if https
     SSL_CTX *s = NULL;
     SSL *tlsc = NULL;


    if(strcmp(url.protocol, "https") == 0)
            {
               s =  sslcont();
               if(s == NULL)
               {
                return 1;
               }
            }

    // zero the structs
    memset(&hints, 0, sizeof(hints));
    // for ipv4 and tcp
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    //
    if ((status = getaddrinfo(url.hostname, url.port, &hints, &res)) != 0)
    {
        fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status));
        return 1;
    }
//try to make a socket and connect

    int sockfd = createsocket(res, ip, sizeof(ip));
    freeaddrinfo(res);

    if(sockfd == -1)
    {
        printf("couldnt create socket\n");
        return 1;
    }

 // if connection is succesful then send the request based on protocol
            if(strcmp(url.protocol, "https") == 0)
            {
             tlsc = createtls(s, sockfd, url.hostname);
             if(tlsc == NULL)
             {
                return 1;
             }
            }

            else
            {
                //todo
            }

          /*  char *download_req = "GET %s HTTP/1.1\r\n"
                                 "Host: %s\r\n"
                                 "Connection: close\r\n"
                                 "\r\n";
            // combining the get template with the inputted url

            size_t buffer_size = strlen(download_req) + strlen(url.path) + strlen(url.hostname) + 1;

            char dom[buffer_size];

            snprintf(dom, sizeof(dom), download_req, url.path, url.hostname);

            // send req

            size_t request_len = strlen(dom);
            int bytes_sent = send(sockfd, dom, request_len, 0);

            if (bytes_sent == -1)
            {
                perror("Failed to send download request");
            }

            printf("socket created succefully! and connection is succesful! file descriptor: %d\n",
                   sockfd);

            break;
        }
    }

    freeaddrinfo(res);
*/
    printf("hostname: %s\nport: %s\npath: %s\n", url.hostname, url.port, url.path);

    return 0;
}



