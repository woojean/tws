#include <stdio.h>  
#include <time.h> 
#include <string.h>
#include <stdlib.h>
#include <sys/socket.h>

#include "base.h"

void test_connect(){
	int fd = open_clientfd("192.168.1.2",88);
	
	// close nagle
	const char opt=1;   
    setsockopt(fd,IPPROTO_TCP,TCP_NODELAY,&opt,sizeof(char));   

	char buf[MAXBUF];
	//char* content = "GET /index.html HTTP/1.1\r\n";
	char* content = "GET /index.php HTTP/1.1\r\n";

	int len = strlen(content);
	int sendret = send(fd,content,len,0);
	int recvret = recv(fd,buf,MAXBUF,0);

	printf("%s\n", buf);
}


void test_php_cmd(){
	char phpbuf[MAXBUF];


    char *cmd = "php /vagrant/www/github/tws/demo/public/index.php";

	FILE *phpfp = popen(cmd,"r"); 
	if(NULL == phpfp){
		sprintf(phpbuf, "php error !");
	}
	else{
		int phpfd  =  fileno(phpfp);
		rio_readn(phpfd, phpbuf, MAXBUF);
	}  
	pclose(phpfp);

	printf("%s\n", phpbuf);
}

int main(void)
{
	test_connect();

}