
//the Architecture is 100% mine ai was only used to stress test, find edge cases, and some small boilerplate code segments 

#define _GNU_SOURCE
#include "parser.h"
#include "impheads.h"
#include "tls.h"
#include "connections.h"
#include "optimal_threadc.h"
#include "multithreading.h"
typedef struct addrinfo as;
typedef struct sockaddr_in net;
int openfile(char *filename, char *content_len);
int workerinfo(int tls, threadinfo *worker, int count, char *content_len, struct parser *url, int fd, SSL_CTX *config, struct addrinfo *ress, char *filesizein);

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


        if(httpresponse.location != NULL)
        {
            printf("the target address has been moved to: %s \nrerun the program using the new address.\n", httpresponse.location);
            goto cleanup;
        }
                if (atoi(httpresponse.status_code) != 200)

       {
            fprintf(stderr, "Server returned HTTP status: %s\n", httpresponse.status_code);
            goto cleanup;
       }
            if (httpresponse.ranged_req != 1)
{
    printf("Server does not support ranged requests. Fallback is not implemented yet.\n");
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
         if(workerinfo(1, workers, nthreads, httpresponse.content_len, &url, fd, s, res, httpresponse.content_len) != 0)
         {
            printf("error in initializing the thread struct!\n");
            close(fd);
            goto cleanup;
         }
         int succesfult = spawnthreads(nthreads, workerfunctls, workers);

         
if (succesfult != nthreads)
{
    printf("error in creating the threads!\n");
    int join_fail = 0;
    for (int i = 0; i < succesfult; i++)
    {

        int rc = pthread_join(workers[i].thread, NULL);
        if (rc != 0)
        {
            fprintf(stderr, "pthread_join failed: %s\n", strerror(rc));
        join_fail = 1;
        }
    }
if (join_fail)
    return 1;

close(fd);
goto cleanup;

}
int join_fail = 0;
for (int i = 0; i < nthreads; i++)
{
    int rc = pthread_join(workers[i].thread, NULL);

    if (rc != 0)
    {
        fprintf(stderr, "pthread_join failed: %s\n", strerror(rc));
join_fail = 1;
  
    }
}
if (join_fail)
    return 1;

int download_success = 1;

for (int i = 0; i < nthreads; i++)
{
    if (workers[i].result != 1)
    {
        fprintf(stderr, "Worker %d failed\n", i);
        download_success = 0;
    }
}

close(fd);

if (!download_success)
    goto cleanup; 

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
                if (atoi(httpresponse.status_code) != 200)

       {
            fprintf(stderr, "Server returned HTTP status: %s\n", httpresponse.status_code);
            goto cleanup;
       }
            if (httpresponse.ranged_req != 1)
{
    printf("Server does not support ranged requests. Fallback is not implemented yet.\n");
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
            goto cleanup;
         }
         threadinfo workers[nthreads];
         if(workerinfo(0, workers, nthreads, httpresponse.content_len, &url, fd, s, res, httpresponse.content_len) != 0)
         {
            printf("error in initializing the thread struct!\n");
            close(fd);
            goto cleanup;
         }
          int succesfult = spawnthreads(nthreads, workerfuncntls, workers);
          if (succesfult != nthreads)
{
    printf("error in creating the threads!\n");
    int join_fail = 0;
    for (int i = 0; i < succesfult; i++)
    {
        int rc = pthread_join(workers[i].thread, NULL);
        if (rc != 0)
        {
            fprintf(stderr, "pthread_join failed: %s\n", strerror(rc));
            join_fail = 1;
        }
    }
if (join_fail)
    return 1;
close(fd);
goto cleanup;


}
int join_fail = 0;

for (int i = 0; i < nthreads; i++)
{
    int rc = pthread_join(workers[i].thread, NULL);

    if (rc != 0)
    {
        fprintf(stderr, "pthread_join failed: %s\n", strerror(rc));
        join_fail = 1;
    }
}

if (join_fail)
    return 1;

int download_success = 1;

for (int i = 0; i < nthreads; i++)
{
    if (workers[i].result != 1)
    {
        fprintf(stderr, "Worker %d failed\n", i);
        download_success = 0;
    }
}

close(fd);

if (!download_success)
    goto cleanup;
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



}










//using fallocate as most linux systems support it
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

int workerinfo(int tls, threadinfo *worker, int count, char *content_len, struct parser *url, int fd, SSL_CTX *config, struct addrinfo *ress, char *filesizein)
{
    if(tls != 1)
    {
        config = NULL;
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


  int64_t chunksize = filesize/count;
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
















