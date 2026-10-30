#include "url_parser.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>



struct parser parse(char *s)
{
    // protocol hostname path port
    struct parser filled = {0};
    filled.success = 0;
    char *t = s;

    // protoco
    int i = 0;
    while (*t != '\0' && (int) *t != 58 && i < 7)
    {
        filled.protocol[i] = *t;
        t = t + 1;
        i++;
    }
    filled.protocol[i] = '\0';
  if (strcmp(filled.protocol, "https") == 0)
{
    strcpy(filled.port, "443");
}
if (strcmp(filled.protocol, "http") == 0)
{
    strcpy(filled.port, "80");
}
    // hostname

    int buffer = 30;
    char *host = malloc(sizeof(char) * buffer);
    if (host == NULL)
    {
        printf("couldnt allocate memory for hostname buffer");
        return filled;
    }
    i = 0;
    if (*t != ':' || t[1] != '/' || t[2] != '/')
    {
        free(host);
        return filled;
    }

    t = t + 3;
    while (*t != '/' && *t != '?' && *t != '#' && *t != '\0')
    {
        if (i == buffer - 1)
        {
            char *temp;
            temp = realloc(host, sizeof(char) * buffer * 2);
            if (temp == NULL)
            {
                free(host);
                printf("couldnt allocate memory for larger hostname");
                return filled;
            }
            buffer *= 2;
            host = temp;
        }
        //if an explicit port is there

        if(*t == ':')
        {
           char port[6];
           t =t+1;
           int l = 0;
           while(*t != '/'  && *t != '?' && *t != '#' && *t != '\0')
           {

            if(l == 5 || *t < '0' || *t > '9')
            {
                free(host);
                printf("invalid port\n");
                return filled;
            }


               port[l] = *t;
               t++;
               l++;

           }
           if(l == 0)
           {
                free(host);
                printf("invalid port\n");
                return filled;
           }



           port[l] = '\0';

             if(l ==5)
           {
            if(strcmp(port, "65535")>0)
            {
                free(host);
                printf("invalid port\n");
                return filled;
            }
           }

          strcpy(filled.port, port);
           break;


        }


        host[i] = *t;
        t = t + 1;
        i++;
    }

    host[i] = '\0';
    filled.hostname = host;



    //path

   while (*t != '\0' && *t != '/' && *t != '?' && *t != '#')
    {
        t += 1;
    }

    if (*t == '\0' || *t == '#')
    {
        filled.path = malloc(2 * sizeof(char));

        if (filled.path == NULL)
        {
            free(filled.hostname);
            filled.hostname = NULL;
            return filled;
        }

        strcpy(filled.path, "/");
    }
    else
    {

        int path = 30;
        int p = 0;
        char *loc = malloc(sizeof(char) * path);
        if (loc == NULL)
        {
            free(filled.hostname);
            filled.hostname = NULL;
            printf("couldnt allocate memory for path\n");
            return filled;
        }
        char *tmp = loc;

        if (*t == '?')
        {
            *tmp = '/';
            tmp++;
            p++;
        }

        while (*t != '#' && *t != '\0')
        {

            if (p == path - 1)
            {
                char *temp;
                temp = realloc(loc, sizeof(char) * path * 2);
                if (temp == NULL)
                {
                    free(loc);
                    free(filled.hostname);
                    filled.hostname = NULL;
                    printf("couldnt allocate memory for reallocation\n");
                    return filled;
                }
                loc = temp;
                tmp = loc + p;
                path *= 2;
            }
            *tmp = *t;
            tmp++;
            t++;
            p++;
        }

        *tmp = '\0';

        filled.path = loc;
    }


    filled.success = 1;
    return filled;
}










int iscorrect(struct parser *url)
{
    if (!url->success || url->hostname == NULL || url->path == NULL)
    return 1;
    char *runner = url->hostname;
    if (*runner == '\0')
    {
        return 1;
    }
    // for the protocol
    if (strcmp(url->protocol, "https") != 0 && strcmp(url->protocol, "http") != 0)
    {
        return 1;
    }
    // for hostname
    if (*runner == '-' || *runner == '.')
    {
        return 1;
    }

    while (*runner != '\0')
    {

        if (*runner <= 'z' && *runner >= 'a')
        {
            runner += 1;
            continue;
        }
        else if (*runner <= 'Z' && *runner >= 'A')
        {
            runner += 1;

            continue;
        }
        else if (*runner <= '9' && *runner >= '0')
        {
            runner += 1;

            continue;
        }
        else if (*runner == '-' || *runner == '.')
        {
            if (*(runner + 1) == '\0' || *(runner + 1) == '-' || *(runner + 1) == '.')
            {
                return 1;
            }
            runner += 1;

            continue;
        }
        else
        {
            return 1;
        }
    }
    // for path

    char *runner2 = url->path;

    while (*runner2 != '\0')
    {
        if (*runner2 == '%')
        {
            char *tmp = hexcheck(runner2);
            if (tmp == NULL)
            {
                return 1;
            }
            runner2 = tmp;
            continue;
        }

        if (*runner2 <= 'z' && *runner2 >= 'a')
        {
            runner2 += 1;
            continue;
        }
        else if (*runner2 <= 'Z' && *runner2 >= 'A')
        {
            runner2 += 1;

            continue;
        }
        else if (*runner2 <= '9' && *runner2 >= '0')
        {
            runner2 += 1;
            continue;
        }

        else if (*runner2 == '/' || *runner2 == '_' || *runner2 == '-' || *runner2 == '.' ||
                 *runner2 == '~' || *runner2 == '?' || *runner2 == '&' || *runner2 == '=' ||
                 *runner2 == '+')
        {
            runner2 += 1;
            continue;
        }
        else
        {
            return 1;
        }
    }

    return 0;
}

// hexcheck

char *hexcheck(char *runner2)
{
    runner2 += 1;
    int count = 1;
    while (*runner2 != '\0' && count <= 2)
    {
        if (*runner2 >= '0' && *runner2 <= '9')
        {
            runner2 += 1;
            count += 1;
            continue;
        }
        else if (*runner2 >= 'a' && *runner2 <= 'f')
        {
            runner2 += 1;
            count += 1;
            continue;
        }
        else if (*runner2 >= 'A' && *runner2 <= 'F')
        {
            runner2 += 1;
            count += 1;
            continue;
        }
        else
        {
            return NULL;
        }
    }
    if (count != 3)
    {
        return NULL;
    }
    return runner2;
}






