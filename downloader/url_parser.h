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



struct parser parse(char *s);
int iscorrect(struct parser *url);
char *hexcheck(char *runner2);

#endif
