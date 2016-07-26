#include <stdio.h>  
#include <time.h> 
#include <string.h> // !! bzero
#include <stdlib.h> // !! atoi

#define MAXBUF   8192  /* Max I/O buffer size */

int main(void)
{
	time_t timep;  
    struct tm *p;  
    time(&timep);  
    p = gmtime(&timep); 
  
    char str[MAXBUF],log_path[MAXBUF];
    sprintf(str,"%d-%d-%d.log",1900+p->tm_year,1+p->tm_mon,p->tm_mday);
    strcat(log_path, str);

    printf("%s\n", log_path);

}