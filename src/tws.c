#include "base.h"

void handle(int fd);
void *handle_wrapper(void *args);
void read_requesthdrs(rio_t *rp);
int  parse_uri(char *uri, char *filename, char *cgiargs);
void serve_static(int fd, char *filename, int filesize);
void get_filetype(char *filename, char *filetype);
void serve_dynamic(int fd, char *filename, char *cgiargs);
void clienterror(int fd, char *cause, char *errnum, char *shortmsg, char *longmsg);
void Log(const char *msg);
void init_server();

int listenfd;
char www_path[MAXBUF], log_path[MAXBUF], port_number[MAXBUF], php[MAXBUF];

int main(int argc, char **argv) 
{
    // parse config
    char* conf;
    char* conf_file = "/conf/tws.conf";
    char cwd[MAXBUF];   
    
    getcwd(cwd,sizeof(cwd)); 
    strcat(cwd,conf_file);
    conf = cwd;
    
    get_conf(conf, "dirs", "www", www_path);
    get_conf(conf, "dirs", "log", log_path);
    get_conf(conf, "globals", "port", port_number);
    get_conf(conf, "extensions", "php", php);

    // init log set
    time_t timep;  
    struct tm *p;  
    time(&timep);  
    p = gmtime(&timep); 
  
    char str[MAXBUF];
    sprintf(str,"/%d-%d-%d.log",1900+p->tm_year,1+p->tm_mon,p->tm_mday);
    strcat(log_path, str);

    Log("Server started !\n======================================\n");

    listenfd = open_listenfd(atoi(port_number));
    if(listenfd < 0){
        error("open listenfd failed !");
    }
    Log("Listening...\n--------------------------------------\n");

    while (1) {
        int connfd, clientlen;
        struct sockaddr_in clientaddr;

        clientlen = sizeof(clientaddr);
        connfd = Accept(listenfd, (struct sockaddr *)&clientaddr, &clientlen);

        /*
        // no threading
        handle(connfd);
        */

        pthread_t pt;
        pthread_create(&pt, NULL, handle_wrapper, &connfd);
        pthread_join(pt, NULL);
    }
}


void *handle_wrapper(void *args)
{
    int fd;
    fd = (int)(*((int*)args));
    handle(fd);
}

void handle(int fd) 
{
    int is_static;
    struct stat sbuf;
    char buf[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE];
    char filename[MAXLINE], cgiargs[MAXLINE];
    
    rio_t rio;
    rio_readinitb(&rio, fd);
    if( rio_readlineb(&rio, buf, MAXLINE) < 0 ){
        error("rio_readlineb error");
    }

    // HTTP request line
    sscanf(buf, "%s %s %s", method, uri, version);

    if (0 != strcasecmp(method, "GET") && 0 != strcasecmp(method, "POST")) {
        Log(method);
        Log(" / Method haven't implemented !\n");
        clienterror(fd, method, 
            "501", 
            "Not Implemented",
            "Tiny does not implement this method");
        return;
    }
    
    is_static = parse_uri(uri, filename, cgiargs);

    if (stat(filename, &sbuf) < 0) {
        Log(filename);
        Log(" / File not exist !\n");
        clienterror(fd, filename, "404", "Not found","file not exist !");
        return;
    }

    if (is_static) {
        if (!(S_ISREG(sbuf.st_mode)) || !(S_IRUSR & sbuf.st_mode)) {
            Log(filename);
            Log(" / No permission to read the file !\n");
            clienterror(fd, filename, "403", "Forbidden","No permission to read the file !");
            return;
        }
        serve_static(fd, filename, sbuf.st_size);
    }
    else {
        if (!(S_ISREG(sbuf.st_mode)) || !(S_IXUSR & sbuf.st_mode)) {
            Log(filename);
            Log(" / Couldn't run the CGI program !\n");
            clienterror(fd, filename, "403", "Forbidden","Couldn't run the CGI program");
            return;
        }
        serve_dynamic(fd, filename, cgiargs);
    }

    if(close(fd) <0){
        // todo log
        Log("Error:Server close failed !\n");
        error(" / Close failed !");
    }
}

int parse_uri(char *uri, char *filename, char *cgiargs) 
{
    char *ptr;

    if (!strstr(uri, ".php")) {  // Static content
        strcpy(cgiargs, "");
        strcpy(filename,www_path);
        strcat(filename, uri);
	   
        if (uri[strlen(uri)-1] == '/')
            strcat(filename, "home.html");
        
        return 1;
    }
    else {  // Dynamic content
        ptr = index(uri, '?');
        if (ptr) {
            strcpy(cgiargs, ptr+1);
            *ptr = '\0';
        }
        else{ 
            strcpy(cgiargs, "");
        }
        strcpy(filename, www_path);
        strcat(filename, uri);
        return 0;
    }
}


void serve_static(int fd, char *filename, int filesize) 
{
    int srcfd;
    char *srcp, filetype[MAXLINE], buf[MAXBUF];
 
    get_filetype(filename, filetype);
    sprintf(buf, "HTTP/1.0 200 OK\r\n");
    sprintf(buf, "%sServer: Tiny Web Server\r\n", buf);
    sprintf(buf, "%sContent-length: %d\r\n", buf, filesize);
    sprintf(buf, "%sContent-type: %s\r\n\r\n", buf, filetype);
    Rio_writen(fd, buf, strlen(buf));

    srcfd = Open(filename, O_RDONLY, 0);
    srcp = Mmap(0, filesize, PROT_READ, MAP_PRIVATE, srcfd, 0);
    if(close(srcfd) <0){
        Log("close failed !");
        error("close failed !");
    }
    Rio_writen(fd, srcp, filesize);
    //strcat(buf,srcp);
    Munmap(srcp, filesize);
}

void serve_dynamic(int fd, char *filename, char *cgiargs) 
{
    char cmd[MAXLINE],phpbuf[MAXLINE], buf[MAXLINE], *emptylist[] = { NULL };

    sprintf(buf, "HTTP/1.0 200 OK\r\n");
    sprintf(buf, "%sServer: Tiny Web Server\r\n\r\n", buf);
    
    sprintf(cmd,"%s %s",php,filename);

    FILE *phpfp = popen(cmd,"r"); 
    if(NULL == phpfp){
        sprintf(phpbuf, "php error !");
    }
    else{
        int phpfd  =  fileno(phpfp);
        rio_readn(phpfd, phpbuf, MAXBUF);
    }  
    pclose(phpfp);

    strcat(buf,phpbuf);
    Rio_writen(fd, buf, strlen(buf));
}

void get_filetype(char *filename, char *filetype) 
{
    if (strstr(filename, ".html")){
	   strcpy(filetype, "text/html");
    }
    else if (strstr(filename, ".gif")){
	   strcpy(filetype, "image/gif");
    }
    else if (strstr(filename, ".jpg")){
	   strcpy(filetype, "image/jpeg");
    }
    else if (strstr(filename, ".png")){
       strcpy(filetype, "image/png");
    }
    else if (strstr(filename, ".gif")){
       strcpy(filetype, "image/gif");
    }
    else if (strstr(filename, ".ico")){
       strcpy(filetype, "image/x-icon");
    }
    else if (strstr(filename, ".css")){
       strcpy(filetype, "text/css");
    }
    else{
	   strcpy(filetype, "text/plain");
    }
}  

void clienterror(int fd, char *cause, char *errnum, 
		 char *shortmsg, char *longmsg) 
{
    char buf[MAXLINE], body[MAXBUF];

    /* Build the HTTP response body */
    sprintf(body, "<html><title>Tiny Error</title>");
    sprintf(body, "%s<body bgcolor=""ffffff"">\r\n", body);
    sprintf(body, "%s%s: %s\r\n", body, errnum, shortmsg);
    sprintf(body, "%s<p>%s: %s\r\n", body, longmsg, cause);
    sprintf(body, "%s<hr><em>The Tiny Web server</em>\r\n", body);

    /* Print the HTTP response */
    sprintf(buf, "HTTP/1.0 %s %s\r\n", errnum, shortmsg);
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "Content-type: text/html\r\n");
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "Content-length: %d\r\n\r\n", (int)strlen(body));
    Rio_writen(fd, buf, strlen(buf));
    Rio_writen(fd, body, strlen(body));
}


void Log(const char *msg){
    FILE *fp;
    fp = fopen(log_path, "a+");
    fputs(msg, fp);
    fputs("\n", fp);
    fclose(fp);
}