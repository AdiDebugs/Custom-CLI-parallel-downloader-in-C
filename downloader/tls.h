#ifndef __TLS_H_
#define __TLS_H_

SSL_CTX * sslcont(void);
SSL *createtls(SSL_CTX *ctx, int sockfd, const char *hostname);


#endif
