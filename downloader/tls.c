#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/x509_vfy.h>
#include "tls.h"






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




