#ifndef __TLS_H_
#define __TLS_H_
#include <openssl/ssl.h>

struct parser;  
SSL_CTX * sslcont(void);
SSL *createtls(SSL_CTX *ctx, int sockfd, const char *hostname);
char *tlshttpresp( SSL *tlsc, struct parser *url);


#endif
