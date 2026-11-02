
#define _GNU_SOURCE
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/x509_vfy.h>
#include "impheads.h"
#include "tls.h"
#include "parser.h"
#define INITIAL_CAPACITY 16384
#define MAX_CAPACITY 65536






SSL_CTX * sslcont(void)
{

    SSL_CTX *ctx = SSL_CTX_new(TLS_client_method()); //automatically setup for a client not manual

    if (ctx == NULL)
    {
        ERR_print_errors_fp(stderr);
        return NULL;
    }

    // Enable certificate verification
    SSL_CTX_set_verify(ctx, SSL_VERIFY_PEER, NULL);

    // Load trusted CA certificates
    if (SSL_CTX_set_default_verify_paths(ctx) != 1)
    {
        ERR_print_errors_fp(stderr);
        SSL_CTX_free(ctx);
        return NULL;
    }

    // Require at least TLS 1.2
    if (SSL_CTX_set_min_proto_version(ctx, TLS1_2_VERSION) != 1)
    {
        ERR_print_errors_fp(stderr);
        SSL_CTX_free(ctx);
        return NULL;
    }

    return ctx;
}



SSL *createtls(SSL_CTX *ctx, int sockfd, const char *hostname)
{
SSL *ssl = SSL_new(ctx);
if (ssl == NULL)
{
        ERR_print_errors_fp(stderr);
        return NULL;
}

if (SSL_set_fd(ssl, sockfd) != 1)
{
        ERR_print_errors_fp(stderr);
        SSL_free(ssl);
        return NULL;
}

    if (SSL_set_tlsext_host_name(ssl, hostname) != 1)
    {
        ERR_print_errors_fp(stderr);
        SSL_free(ssl);
        return NULL;
    }



        // Configure hostname verification
    if (SSL_set1_host(ssl, hostname) != 1)
    {
        ERR_print_errors_fp(stderr);
        SSL_free(ssl);
        return NULL;
    }

//perform tls handshake
    int ret = SSL_connect(ssl);

    if (ret != 1)
    {
        int error = SSL_get_error(ssl, ret);

        fprintf(stderr, "TLS handshake failed. SSL error: %d\n", error);

        long verify_result = SSL_get_verify_result(ssl);

        if (verify_result != X509_V_OK)
        {
            fprintf(stderr, "Certificate verification failed: %s\n",
                    X509_verify_cert_error_string(verify_result));
        }

        ERR_print_errors_fp(stderr);
        SSL_free(ssl);
        return NULL;
    }

printf("TLS handshake successful!\n");

    printf("TLS version: %s\n", SSL_get_version(ssl));

    return ssl;
}


//head request tls


char *tlshttpresp( SSL *tlsc, struct parser *url, struct timespec *start_time, struct timespec *end_time)

{
     

             char *headreq = "HEAD %s HTTP/1.1\r\n"
                             "Host: %s\r\n"
                             "Connection: close\r\n\r\n";
             
int needed = snprintf(NULL, 0, headreq, url->path, url->hostname);

if (needed < 0)
{
 
    return NULL;
}

size_t buffer_size = (size_t)needed + 1;
char buffer[buffer_size];

snprintf(buffer, buffer_size, headreq, url->path, url->hostname);
size_t actual_written = 0;

             size_t request_len = strlen(buffer);
             while(1)
             {

             
            size_t bytes_written = SSL_write(tlsc, buffer + actual_written, (int)(request_len-actual_written));
            
                if (bytes_written <= 0)
             {
                printf("couldnt transmit data to the server\n");
                return NULL;
             }
             
            actual_written += bytes_written;
            

            if(actual_written == request_len)
             {
              break;
             }
                

         

             }
size_t capacity = INITIAL_CAPACITY;


char *reciev = malloc(sizeof(char)*(capacity+1));
if(reciev == NULL)
{
    printf("couldnt allocate memory fot the server resp \n");

    return NULL;
}
size_t total_read = 0;



if (clock_gettime(CLOCK_MONOTONIC, &start_time) != 0) 
{
    printf("couldnt calculate the download speed\n");
}
while (1)
{

if (total_read == capacity)
    {
        if (capacity >= MAX_CAPACITY)
        {
            fprintf(stderr, "response payload too large\n");
            free(reciev);
            return NULL;
        }

        size_t new_capacity = capacity * 2;
        if (new_capacity > MAX_CAPACITY)
            new_capacity = MAX_CAPACITY;

        char *temp = realloc(reciev, new_capacity + 1);
        if (temp == NULL)
        {
            free(reciev);
            return NULL;
        }
        reciev = temp;
        capacity = new_capacity;
    }

    int bytes_read = SSL_read(tlsc, reciev + total_read,(int)(capacity - total_read));
    if (bytes_read <= 0)
    {
        fprintf(stderr, "SSL error code: %d\n", SSL_get_error(tlsc, bytes_read));
        ERR_print_errors_fp(stderr);
        free(reciev);
        return NULL;
    }

    total_read += (size_t)bytes_read;
    reciev[total_read] = '\0';

    if (strstr(reciev, "\r\n\r\n") != NULL)
    {
if (clock_gettime(CLOCK_MONOTONIC, &end_time) != 0) 
{
    printf("couldnt calculate the download speed\n");
}
        return reciev;
    }
}
}