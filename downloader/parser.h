#ifndef __PARSER_H_
#define __PARSER_H_

struct parser
{
    char protocol[8];
    char *hostname;
    char *path;
    char port[6];
    int success; // 1 for success, 0 for failure
} ;

struct http_parser
{
 int ranged_req;  // 1 for compatible w multi threading, 0 for single threaded
 char *content_len;
 char *status_code;
 char *location; //incase of redirec 3xx status code

};


int httpparse(struct http_parser *serv, char *httpresp);
struct parser parse(char *s);
int iscorrect(struct parser *url);
char *hexcheck(char *runner2);

#endif
