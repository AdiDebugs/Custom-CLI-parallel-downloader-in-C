
/*19/11/26 added contexes for each worker, added a helper to intialize the contex struct and divided the file offset
next steps should to be add a helper to spwan those workers and the routine that writes the actual data, ill call pthread_join in main to let everyworker finsih its work
i also need to handle partial writes sends and error paths in the routine*/

#define _GNU_SOURCE
#include "parser.h"
#include "impheads.h"
#include "tls.h"
#include "connections.h"
#include "optimal_threadc.h"
typedef struct addrinfo as;
typedef struct sockaddr_in net;
int openfile(char *filename, char *content_len);

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

    freeaddrinfo(res);
    res = NULL;

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
         threadinfo workers[nthreads] = {0};
         printf("number of optimal threads is: %d\n", nthreads);
         
         //create the master file 
         int fd = openfile(url.filename, httpresponse.content_len);
         if(fd == -1)
         {
            printf("error while creating the file!\n");
            close(fd);
            goto cleanup;
         }
         if(workerinfo(workers, nthreads, url.content_len, &url, fd, &s) != 0)
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

int workerinfo(threadinfo *worker, int count, char *content_len, struct parser *url, int fd, SSL_CTX *config)
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
  }
  worker[count-1].start_offset = endoffset;
  worker[count-1].end_offset = filesize;
    worker[count-1].output_fd = fd;
      worker[count-1].ctx = config;
      worker[count-1].url = url;

      return 0;
}



//worker spawner

int spawnthreads(int count, void *routine, threadinfo *workerinfo)