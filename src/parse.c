#include <stdio.h>
#include <sys/stat.h>

#include "base.h"


/*
parse config
just for test
*/

int main() 
{	
	char* filename = "/vagrant/www/github/tws/conf/tws.conf";
    rio_t rio;
    
    char buf[MAXLINE];

    int fd = open(filename, O_RDONLY, 0);

    Rio_readinitb(&rio, fd);
    Rio_readlineb(&rio, buf, MAXLINE);

    printf("%s\n", buf);

}

