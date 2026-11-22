


#define _GNU_SOURCE
#include "parser.h"
#include "impheads.h"
#include "tls.h"
#include "connections.h"
#include "optimal_threadc.h"
#define MAX_HEADER_SIZE (32 * 1024)
typedef struct addrinfo as;
typedef struct sockaddr_in net;
int openfile(char *filename, char *content_len);
int workerinfo(threadinfo *worker, int count, char *content_len, struct parser *url, int fd, SSL_CTX *config, struct addrinfo *ress, char *filesizein);
void *workerfunctls(void *arg);

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
    char *notlsservresp = NULL;

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

 

    if (sockfd == -1)
    {
        printf("couldnt create socket\n");
        goto cleanup;
    }

    

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
        if (atoi(httpresponse.status_code) != 200)

       {
            fprintf(stderr, "Server returned HTTP status: %s\n", httpresponse.status_code);
            goto cleanup;
       }

        if(httpresponse.location != NULL)
        {
            printf("the target address has been moved to: %s \nrerun the program using the new address.\n", httpresponse.location);
            goto cleanup;
        }

       if(httpresponse.ranged_req == 1)
       {
         double resptime = transfer_time(&start_time, &end_time);
         int nthreads = optimalnum(resptime, httpresponse.content_len, argv[2]);
         //array of structs
         threadinfo workers[nthreads];
         printf("number of optimal threads is: %d\n", nthreads);
         
         //create the master file 
         int fd = openfile(url.filename, httpresponse.content_len);
         if(fd == -1)
         {
            printf("error while creating the file!\n");
            goto cleanup;
         }
         if(workerinfo(workers, nthreads, httpresponse.content_len, &url, fd, s, res, httpresponse.content_len) != 0)
         {
            printf("error in initializing the thread struct!\n");
            close(fd);
            goto cleanup;
         }
         //first cret the master file-> divide workers-> pass in info for each worker(using a struct)->use pwrite using a loop-> exit safely
         //one function will spawn the threads and return the thread ids (ill declare an array of the structs based on number of optimal thread count) then pass in the array and spawn the threads in the function, fill the thread ids then use the another helper to open the sockets and writing file
         //implement multithreading



       }

       //fallback to singlethreaded process


    }



    
    else
    {       
//proceed with no tls
notlsservresp = notls(&url, &start_time, &end_time, sockfd);
        if (notlsservresp == NULL)
        {
            printf("failed to recieve server response\n");
            goto cleanup;
        }    
        
        if (httpparse(&httpresponse, notlsservresp) == -1)
        {
            printf("Error in response recieved from the server\n");
            goto cleanup;
        }

         if(httpresponse.location != NULL)
        {
            printf("the target address has been moved to: %s \nrerun the program using the new address.\n", httpresponse.location);
            goto cleanup;
        }

       if(httpresponse.ranged_req == 1)
       {
         double resptime = transfer_time(&start_time, &end_time);
         int nthreads = optimalnum(resptime, httpresponse.content_len, argv[2]);
         printf("number of optimal threads is: %d\n", nthreads);
         //implement multithreading

         //create the master file 
         int fd = openfile(url.filename, httpresponse.content_len);
         if(fd == -1)
         {
            printf("error while creating the file!\n");
            close(fd);
            goto cleanup;
         }

         //send the get req


       }

    }






    printf("hostname: %s\nport: %s\npath: %s\n", url.hostname, url.port, url.path);
    printf("range supported? %d\ncontent length: %s \nstatus code: %s\nredirect location(if any) %s\nfile name: %s\n", httpresponse.ranged_req, httpresponse.content_len, httpresponse.status_code, httpresponse.location, url.filename);
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
free(notlsservresp);
free(httpresponse.content_range);
    return exit_status;
}










//using fallocate for linux
int openfile(char *filename, char *content_len)
{
    if (filename == NULL || content_len == NULL)
    {
        return -1;
    }

    errno = 0;
    char *endptr = NULL;
    int64_t filesize = strtoll(content_len, &endptr, 10);

    if (errno == ERANGE)
    {
        fprintf(stderr, "Filesize exceeds system limit.\n");
        return -1;
    }

    if (endptr == content_len || *endptr != '\0' || filesize < 0)
    {
        fprintf(stderr, "Invalid filesize.\n");
        return -1;
    }

    int fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd == -1)
    {
        perror("openint file failed");
        return -1;
    }

off_t target_size = filesize;
int res;

if (target_size > 0) 
{
    do 
    {
        res = fallocate(fd, 0, 0, target_size);
    } while (res == -1 && errno == EINTR);

    if (res == -1)
     {
        if (errno == EOPNOTSUPP || errno == ENOSYS) 
        {
            do {
                res = ftruncate(fd, target_size);
            } while (res == -1 && errno == EINTR);

            if (res == -1) 
            {
                perror("ftruncate fallback failed");
                close(fd);
                return -1;
            }
        } 
        else
         {
            perror("fallocate failed");
            close(fd);
            return -1;
        }
    }
}

return fd;
}





//intialize the stucts 

int workerinfo(threadinfo *worker, int count, char *content_len, struct parser *url, int fd, SSL_CTX *config, struct addrinfo *ress, char *filesizein)
{
    
    errno = 0;
    char *endptr = NULL;
    int64_t filesize = strtoll(content_len, &endptr, 10);

    if (errno == ERANGE)
    {
        fprintf(stderr, "Filesize exceeds system limit.\n");
        return -1;
    }

    if (endptr == content_len || *endptr != '\0' || filesize < 0)
    {
        fprintf(stderr, "Invalid filesize.\n");
        return -1;
    }


  int64_t chunksize = filesize/count;
  int success = 0;
off_t startoffset = 0;
off_t endoffset = 0;
  for(int i = 0; i< count-1; i++)
  {
      startoffset = endoffset;
      endoffset += chunksize;
      worker[i].start_offset = startoffset;
      worker[i].end_offset = endoffset;
      worker[i].output_fd = fd;
      worker[i].ctx = config;
      worker[i].url = url;
      worker[i].res = ress;
      worker[i].file_size = filesizein;
      worker[i].result = 0;
  }
  worker[count-1].start_offset = endoffset;
  worker[count-1].end_offset = filesize;
    worker[count-1].output_fd = fd;
      worker[count-1].ctx = config;
      worker[count-1].url = url;
            worker[count-1].res = ress;
      worker[count-1].file_size = filesizein;
      worker[count-1].result = 0;

      return 0;
}





//worker routine
void *workerfunctls(void *arg)
{
    struct http_parser headerr = {0};
        threadinfo *workerinfo = (threadinfo *)arg;  
        char *req_buf = NULL;

    off_t write_offset = workerinfo->start_offset;
     SSL *tlsc = NULL;

  int sockfd = createsocket(workerinfo->res, workerinfo->ip, sizeof(workerinfo->ip));
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

    int bytes_read = SSL_read(tlsc, header_buf + total_read, (int)(MAX_HEADER_SIZE - total_read));
    if (bytes_read <= 0)
     {
        printf("SSL error code: %d\n", SSL_get_error(tlsc, bytes_read));
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
char startoffsetresp[MAX_HEADER_SIZE];
char endoffsetresp[MAX_HEADER_SIZE];
char contentlenres[MAX_HEADER_SIZE];

char *runnercr = headerr.content_range;
if (strncasecmp(runnercr, "bytes ", 6) != 0)
{
   goto cleanup;
}
runnercr += 6;
int i = 0;

// Read start offset
while (*runnercr >= '0' && *runnercr <= '9' && i < MAX_HEADER_SIZE - 1)
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

while (*runnercr >= '0' && *runnercr <= '9' && i < MAX_HEADER_SIZE - 1)
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

while (*runnercr >= '0' && *runnercr <= '9' && i < MAX_HEADER_SIZE - 1)
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
        printf("SSL error code: %d\n", SSL_get_error(tlsc, bytes_read));
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
        SSL_free(tlsc);

    free(req_buf);

    free(headerr.status_code);
    free(headerr.content_len);
    free(headerr.location);
    free(headerr.content_range);

    if (sockfd != -1)
        close(sockfd);

    return NULL;
}

//worker spawner

//int spawnthreads(int count, void *routine, threadinfo *workerinfo)