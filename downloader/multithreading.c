
#include "impheads.h"
#include "optimal_threadc.h"
#include "parser.h"
#include "connections.h"
#include "tls.h"
#include "multithreading.h"
#define MAX_HEADER_SIZE (32 * 1024)




//worker spawner

int spawnthreads(int count, void *(*routine)(void *), threadinfo *workerinfo)
{
    int tcount = 0;
 for (int i = 0; i < count; i++)
    {
        if (pthread_create(&workerinfo[i].thread, NULL, routine, &workerinfo[i]) == 0)
        {
           tcount++;
        }
        else
        {
            return tcount;
        }
    }
    return tcount; 
}


//for http


void *workerfuncntls(void *arg)
{
    threadinfo *workerinfo = (threadinfo *)arg;
    int sockfd = -1;  

   struct http_parser headerr = {0};
        char *req_buf = NULL;

    off_t write_offset = workerinfo->start_offset;
sockfd = createsocket(workerinfo->res, workerinfo->ip, sizeof(workerinfo->ip));
  if (sockfd == -1)
  {
    printf("couldnt create socket for the workers\n");
    goto cleanup;
  }


   char *get_req = "GET %s HTTP/1.1\r\n"
                    "Host: %s\r\n"
                    "Range: bytes=%jd-%jd\r\n\r\n";

  
  
int needed = snprintf(NULL, 0, get_req, workerinfo->url->path, workerinfo->url->hostname, (intmax_t)workerinfo->start_offset, (intmax_t)workerinfo->end_offset-1);



//construct get reqest
req_buf = malloc(sizeof(char)*needed +1);
if(req_buf == NULL)
{
 printf("error in allocating size for get buffer\n");
 goto cleanup;
}
snprintf(req_buf, needed + 1, get_req, workerinfo->url->path, workerinfo->url->hostname, (intmax_t)workerinfo->start_offset, (intmax_t)workerinfo->end_offset-1);

//sending

ssize_t actual_written = 0;

             ssize_t request_len = strlen(req_buf);
             while(1)
             {

             
            ssize_t bytes_written = write(sockfd, req_buf + actual_written, (int)(request_len-actual_written));
                        if (bytes_written < 0) 
            {
    if (errno == EINTR) continue;
    goto cleanup;              
}
            
                if (bytes_written <= 0)
             {
                printf("couldnt transmit data to the server\n");
                goto cleanup;
             }
             
            actual_written += bytes_written;
            

            if(actual_written == request_len)
             {
              break;
             }

             }

//recieving header 
char header_buf[MAX_HEADER_SIZE + 1];
size_t total_read = 0;
char *delim_pos = NULL;

while (1) 
{
    if (total_read == MAX_HEADER_SIZE) 
    {
        fprintf(stderr, "response payload too large\n");
        goto cleanup;
    }

    int bytes_read = read(sockfd, header_buf + total_read, (int)(MAX_HEADER_SIZE - total_read));
                if (bytes_read < 0) 
            {
    if (errno == EINTR) continue;
    goto cleanup;              
}
    if (bytes_read <= 0)
     {
        printf("error in reading\n");
        goto cleanup;
    }

    total_read += (size_t)bytes_read;
    header_buf[total_read] = '\0';

//delimeter pos
    delim_pos = strstr(header_buf, "\r\n\r\n");
    if (delim_pos != NULL)
     {
        break;
    }
}

// Point to the start of any body bytes that arrived accidentally
char *body_start = delim_pos + 4;
size_t header_bytes = (size_t)(body_start - header_buf);
size_t leftover_bytes = total_read - header_bytes;



if (httpparse(&headerr, header_buf) == -1)
{
    printf("error in parsing the GET response in workers\n");
    goto cleanup;
}

// Validate response
if (strcmp(headerr.status_code, "206") != 0)
{
    printf("HTTP code mismatch with 206\n");
    goto cleanup;
}

// Check required headers
if (headerr.content_len == NULL || headerr.content_range == NULL)
{
    printf("Required response headers missing\n");
   goto cleanup;
}

// Validate Content-Length
errno = 0;
off_t contentlen = strtoll(headerr.content_len, NULL, 10);

if (errno == ERANGE)
{
    printf("Malformed server response\n");
    goto cleanup;
}

if (contentlen != workerinfo->end_offset - workerinfo->start_offset)
{
    printf("Content length mismatch between server response and assigned length\n");
    goto cleanup;
}

// Parse Content-Range
char startoffsetresp[32];
char endoffsetresp[32];
char contentlenres[32];

char *runnercr = headerr.content_range;
if (strncasecmp(runnercr, "bytes ", 6) != 0)
{
   goto cleanup;
}
runnercr += 6;
int i = 0;

// Read start offset
while (*runnercr >= '0' && *runnercr <= '9' && i <31)
{
    startoffsetresp[i++] = *runnercr++;
}
startoffsetresp[i] = '\0';

if (i == 0 || *runnercr != '-')
{
    printf("Malformed Content-Range start offset\n");
    goto cleanup;
}


runnercr++;

// Read end offset
i = 0;

while (*runnercr >= '0' && *runnercr <= '9' && i < 31)
{
    endoffsetresp[i++] = *runnercr++;
}
endoffsetresp[i] = '\0';

if (i == 0 || *runnercr != '/')
{
    printf("Malformed Content-Range end offset\n");
   goto cleanup;
}


runnercr++;

//  total resource size
i = 0;

while (*runnercr >= '0' && *runnercr <= '9' && i < 31)
{
    contentlenres[i++] = *runnercr++;
}
contentlenres[i] = '\0';

if (i == 0 || *runnercr != '\0')
{
    printf("Malformed Content-Range total size\n");
   goto cleanup;
}

// Compare total resource size
if (strcmp(contentlenres, workerinfo->file_size) != 0)
{
    printf("Destination file size doesn't match GET response file size\n");
   goto cleanup;
}


errno = 0;
off_t startoffsetrespo = strtoll(startoffsetresp, NULL, 10);

if (errno == ERANGE)
{
    printf("Malformed response: start offset exceeds range\n");
    goto cleanup;
}

off_t endoffsetrespo = strtoll(endoffsetresp, NULL, 10);

if (errno == ERANGE)
{
    printf("Malformed response: end offset exceeds range\n");
    goto cleanup;
}

// Check exact byte ranges
if (startoffsetrespo != workerinfo->start_offset)
{
    printf("Byte range mismatch in start offset\n");
   goto cleanup;
}

if (endoffsetrespo != workerinfo->end_offset - 1)
{
    printf("Byte range mismatch in end offset\n");
   goto cleanup;
}

//write leftover bytes
if(leftover_bytes>contentlen)
{
    printf("too big of a file size\n");
 goto cleanup;
}
size_t written_total = 0;



while (written_total < leftover_bytes)
{
    ssize_t n = pwrite(workerinfo->output_fd, body_start + written_total, leftover_bytes - written_total, write_offset + written_total);

    if (n < 0)
    {
        if (errno == EINTR)
        {
            continue;
        }
        printf("pwrite error\n");
       goto cleanup;
    }

    if (n == 0)
    {
        goto cleanup;
    }
    written_total += (size_t)n;
}
    write_offset += (off_t)written_total;



    off_t bytes_received = leftover_bytes;

//recieving body bytes

//recieving body bytes
char body_buf[32768];

while (bytes_received < contentlen)
{
    size_t to_read = (size_t)(contentlen - bytes_received);
    if (to_read > sizeof(body_buf))
    {
        to_read = sizeof(body_buf);
    }

    int bytes_read = read(sockfd, body_buf, (int)to_read);
if (bytes_read < 0)
{
    if (errno == EINTR)
        continue;
    printf("error in reading body bytes\n");
    goto cleanup;
}
if (bytes_read == 0)
{
    printf(" EOF while reading body bytes\n");
    goto cleanup;
}
    size_t written_total = 0;

    while (written_total < (size_t)bytes_read)
    {
        ssize_t n = pwrite(workerinfo->output_fd, body_buf + written_total, (size_t)bytes_read - written_total, write_offset + written_total);

        if (n < 0)
        {
            if (errno == EINTR)
            {
                continue;
            }
            printf("pwrite error\n");
           goto cleanup;
        }

        if (n == 0)
        {
           goto cleanup;
        }
        written_total += (size_t)n;
    }

    write_offset += (off_t)written_total;
    bytes_received += (off_t)bytes_read;
}
workerinfo->result = 1;

cleanup:


    free(req_buf);

    free(headerr.status_code);
    free(headerr.content_len);
    free(headerr.location);
    free(headerr.content_range);

    if (sockfd != -1)
        close(sockfd);

    return NULL;
}

//for https



//worker routine
void *workerfunctls(void *arg)
{
    struct http_parser headerr = {0};
        threadinfo *workerinfo = (threadinfo *)arg;  
        char *req_buf = NULL;
int sockfd = -1;
    off_t write_offset = workerinfo->start_offset;
     SSL *tlsc = NULL;

  sockfd = createsocket(workerinfo->res, workerinfo->ip, sizeof(workerinfo->ip));
  if (sockfd == -1)
  {
    printf("couldnt create socket for the workers\n");
    goto cleanup;
  }
        tlsc = createtls(workerinfo->ctx, sockfd, workerinfo->url->hostname);

        if (tlsc == NULL)
        {
            printf("TLS initialization failed\n");
           goto cleanup;
            
        }

   char *get_req = "GET %s HTTP/1.1\r\n"
                    "Host: %s\r\n"
                    "Range: bytes=%jd-%jd\r\n\r\n";

  
  
int needed = snprintf(NULL, 0, get_req, workerinfo->url->path, workerinfo->url->hostname, (intmax_t)workerinfo->start_offset, (intmax_t)workerinfo->end_offset-1);



//construct get reqest
req_buf = malloc(sizeof(char)*needed +1);
if(req_buf == NULL)
{
 printf("error in allocating size for get buffer\n");
 goto cleanup;
}
snprintf(req_buf, needed + 1, get_req, workerinfo->url->path, workerinfo->url->hostname, (intmax_t)workerinfo->start_offset, (intmax_t)workerinfo->end_offset-1);

//sending

ssize_t actual_written = 0;

             ssize_t request_len = strlen(req_buf);
             while(1)
             {

             
            ssize_t bytes_written = SSL_write(tlsc, req_buf + actual_written, (int)(request_len-actual_written));
if (bytes_written <= 0) 
{
    int ssl_err = SSL_get_error(tlsc, bytes_written);
    if (ssl_err == SSL_ERROR_WANT_READ || ssl_err == SSL_ERROR_WANT_WRITE) 
    {
        continue;
    }
    if (ssl_err == SSL_ERROR_SYSCALL && bytes_written < 0 && errno == EINTR) 
    {
        continue;
    }
    printf("SSL_read failed, SSL error: %d\n", ssl_err);
    goto cleanup;
}
            
             
            actual_written += bytes_written;
            

            if(actual_written == request_len)
             {
              break;
             }

             }

//recieving header 
char header_buf[MAX_HEADER_SIZE + 1];
size_t total_read = 0;
char *delim_pos = NULL;

while (1) 
{
    if (total_read == MAX_HEADER_SIZE) 
    {
        fprintf(stderr, "response payload too large\n");
        goto cleanup;
    }

    int bytes_read = SSL_read(tlsc, header_buf + total_read, (int)(MAX_HEADER_SIZE - total_read));
   if (bytes_read <= 0) 
   {
    int ssl_err = SSL_get_error(tlsc, bytes_read);
    if (ssl_err == SSL_ERROR_WANT_READ || ssl_err == SSL_ERROR_WANT_WRITE) 
    {
        continue;
    }
    if (ssl_err == SSL_ERROR_SYSCALL && bytes_read < 0 && errno == EINTR) 
    {
        continue;
    }
    printf("SSL_read failed, SSL error: %d\n", ssl_err);
    goto cleanup;
}
    total_read += (size_t)bytes_read;
    header_buf[total_read] = '\0';

//delimeter pos
    delim_pos = strstr(header_buf, "\r\n\r\n");
    if (delim_pos != NULL)
     {
        break;
    }
}

// Point to the start of any body bytes that arrived accidentally
char *body_start = delim_pos + 4;
size_t header_bytes = (size_t)(body_start - header_buf);
size_t leftover_bytes = total_read - header_bytes;



if (httpparse(&headerr, header_buf) == -1)
{
    printf("error in parsing the GET response in workers\n");
    goto cleanup;
}

// Validate response
if (strcmp(headerr.status_code, "206") != 0)
{
    printf("HTTP code mismatch with 206\n");
    goto cleanup;
}

// Check required headers
if (headerr.content_len == NULL || headerr.content_range == NULL)
{
    printf("Required response headers missing\n");
   goto cleanup;
}

// Validate Content-Length
errno = 0;
off_t contentlen = strtoll(headerr.content_len, NULL, 10);

if (errno == ERANGE)
{
    printf("Malformed server response\n");
    goto cleanup;
}

if (contentlen != workerinfo->end_offset - workerinfo->start_offset)
{
    printf("Content length mismatch between server response and assigned length\n");
    goto cleanup;
}

// Parse Content-Range
char startoffsetresp[32];
char endoffsetresp[32];
char contentlenres[32];

char *runnercr = headerr.content_range;
if (strncasecmp(runnercr, "bytes ", 6) != 0)
{
   goto cleanup;
}
runnercr += 6;
int i = 0;

// Read start offset
while (*runnercr >= '0' && *runnercr <= '9' && i < 31)
{
    startoffsetresp[i++] = *runnercr++;
}
startoffsetresp[i] = '\0';

if (i == 0 || *runnercr != '-')
{
    printf("Malformed Content-Range start offset\n");
    goto cleanup;
}


runnercr++;

// Read end offset
i = 0;

while (*runnercr >= '0' && *runnercr <= '9' && i < 31)
{
    endoffsetresp[i++] = *runnercr++;
}
endoffsetresp[i] = '\0';

if (i == 0 || *runnercr != '/')
{
    printf("Malformed Content-Range end offset\n");
   goto cleanup;
}


runnercr++;

//  total resource size
i = 0;

while (*runnercr >= '0' && *runnercr <= '9' && i < 31)
{
    contentlenres[i++] = *runnercr++;
}
contentlenres[i] = '\0';

if (i == 0 || *runnercr != '\0')
{
    printf("Malformed Content-Range total size\n");
   goto cleanup;
}

// Compare total resource size
if (strcmp(contentlenres, workerinfo->file_size) != 0)
{
    printf("Destination file size doesn't match GET response file size\n");
   goto cleanup;
}


errno = 0;
off_t startoffsetrespo = strtoll(startoffsetresp, NULL, 10);

if (errno == ERANGE)
{
    printf("Malformed response: start offset exceeds range\n");
    goto cleanup;
}

off_t endoffsetrespo = strtoll(endoffsetresp, NULL, 10);

if (errno == ERANGE)
{
    printf("Malformed response: end offset exceeds range\n");
    goto cleanup;
}

// Check exact byte ranges
if (startoffsetrespo != workerinfo->start_offset)
{
    printf("Byte range mismatch in start offset\n");
   goto cleanup;
}

if (endoffsetrespo != workerinfo->end_offset - 1)
{
    printf("Byte range mismatch in end offset\n");
   goto cleanup;
}

//write leftover bytes
if(leftover_bytes>contentlen)
{
    printf("too big of a file size\n");
 goto cleanup;
}
size_t written_total = 0;



while (written_total < leftover_bytes)
{
    ssize_t n = pwrite(workerinfo->output_fd, body_start + written_total, leftover_bytes - written_total, write_offset + written_total);

    if (n < 0)
    {
        if (errno == EINTR)
        {
            continue;
        }
        printf("pwrite error\n");
       goto cleanup;
    }

    if (n == 0)
    {
        goto cleanup;
    }
    written_total += (size_t)n;
}
    write_offset += (off_t)written_total;



    off_t bytes_received = leftover_bytes;

//recieving body bytes

//recieving body bytes
char body_buf[32768];

while (bytes_received < contentlen)
{
    size_t to_read = (size_t)(contentlen - bytes_received);
    if (to_read > sizeof(body_buf))
    {
        to_read = sizeof(body_buf);
    }

    int bytes_read = SSL_read(tlsc, body_buf, (int)to_read);
if (bytes_read <= 0) 
{
    int ssl_err = SSL_get_error(tlsc, bytes_read);
    if (ssl_err == SSL_ERROR_WANT_READ || ssl_err == SSL_ERROR_WANT_WRITE) 
    {
        continue;
    }
    if (ssl_err == SSL_ERROR_SYSCALL && bytes_read < 0 && errno == EINTR) 
    {
        continue;
    }
    printf("SSL_read failed, SSL error: %d\n", ssl_err);
    goto cleanup;
}
    size_t written_total = 0;

    while (written_total < (size_t)bytes_read)
    {
        ssize_t n = pwrite(workerinfo->output_fd, body_buf + written_total, (size_t)bytes_read - written_total, write_offset + written_total);

        if (n < 0)
        {
            if (errno == EINTR)
            {
                continue;
            }
            printf("pwrite error\n");
           goto cleanup;
        }

        if (n == 0)
        {
           goto cleanup;
        }
        written_total += (size_t)n;
    }

    write_offset += (off_t)written_total;
    bytes_received += (off_t)bytes_read;
}
workerinfo->result = 1;

cleanup:

    if (tlsc != NULL)
    {
    SSL_shutdown(tlsc);
        SSL_free(tlsc);
    }
    free(req_buf);

    free(headerr.status_code);
    free(headerr.content_len);
    free(headerr.location);
    free(headerr.content_range);

    if (sockfd != -1)
        close(sockfd);

    return NULL;
}



