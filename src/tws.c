#include "base.h"

/* function declare */
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

char www_path[MAXBUF],log_path[MAXBUF],port_number[MAXBUF];

int main(int argc, char **argv) 
{
    int listenfd, connfd, clientlen;

    struct sockaddr_in clientaddr;

    char* conf;
    char* conf_file = "/conf/tws.conf";
    char cwd[MAXBUF];   

    getcwd(cwd,sizeof(cwd)); 
    strcat(cwd,conf_file);

    conf = cwd;

    
    get_conf(conf, "dirs", "www", www_path);
    get_conf(conf, "dirs", "log", log_path);
    get_conf(conf, "globals", "port", port_number);

    init_server();
    Log("XXXXX");

    listenfd = open_listenfd(atoi(port_number));
    if(listenfd < 0){
        error("open listenfd failed !");
    }

    while (1) {
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


void init_server(){
    time_t timep;  
    struct tm *p;  
    time(&timep);  
    p = gmtime(&timep); 
  
    char str[MAXBUF];
    sprintf(str,"/%d-%d-%d.log",1900+p->tm_year,1+p->tm_mon,p->tm_mday);

    strcat(log_path, str);
    printf("%s\n", log_path);
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

    if (0 != strcasecmp(method, "GET")  
        && 0 != strcasecmp(method, "POST")) {
        clienterror(fd, method, 
            "501", 
            "Not Implemented",
            "Tiny does not implement this method");
        return;
    }
    
    read_requesthdrs(&rio);  // just print

    is_static = parse_uri(uri, filename, cgiargs);
    if (stat(filename, &sbuf) < 0) {
        clienterror(fd, filename, "404", "Not found","file not exist !");
        return;
    }

    if (is_static) {
        if (!(S_ISREG(sbuf.st_mode)) || !(S_IRUSR & sbuf.st_mode)) {
            clienterror(fd, filename, "403", "Forbidden","No permission to read the file !");
            return;
        }
        serve_static(fd, filename, sbuf.st_size);
    }
    else {
        if (!(S_ISREG(sbuf.st_mode)) || !(S_IXUSR & sbuf.st_mode)) {
            clienterror(fd, filename, "403", "Forbidden","Couldn't run the CGI program");
            return;
        }
        serve_dynamic(fd, filename, cgiargs);
    }

    if(close(fd) <0){
        // todo log
        error("close failed !");
    }
}


void read_requesthdrs(rio_t *rp) 
{
    char buf[MAXLINE];

    Rio_readlineb(rp, buf, MAXLINE);
    while(strcmp(buf, "\r\n")) {
	   Rio_readlineb(rp, buf, MAXLINE);
	   printf("%s", buf);
    }
    return;
}

/*
 * parse_uri - parse URI into filename and CGI args
 *             return 0 if dynamic content, 1 if static
 */
int parse_uri(char *uri, char *filename, char *cgiargs) 
{
    char *ptr;

    if (!strstr(uri, "cgi-bin")) {  // Static content
        strcpy(cgiargs, "");
        //strcpy(filename, ".");  // !!
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
        strcpy(filename, ".");
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
        error("close failed !");
    }
    Rio_writen(fd, srcp, filesize);
    Munmap(srcp, filesize);
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
    else if (strstr(filename, ".ico")){
       strcpy(filetype, "image/x-icon");
    }
    else{
	   strcpy(filetype, "text/plain");
    }
}  

/*
 * serve_dynamic - run a CGI program on behalf of the client
 */
/* $begin serve_dynamic */
void serve_dynamic(int fd, char *filename, char *cgiargs) 
{
    char buf[MAXLINE], *emptylist[] = { NULL };

    /* Return first part of HTTP response */
    sprintf(buf, "HTTP/1.0 200 OK\r\n"); 
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "Server: Tiny Web Server\r\n");
    Rio_writen(fd, buf, strlen(buf));
  
    if (Fork() == 0) { /* child */ //line:netp:servedynamic:fork
	/* Real server would set all CGI vars here */
	setenv("QUERY_STRING", cgiargs, 1); //line:netp:servedynamic:setenv
	Dup2(fd, STDOUT_FILENO);         /* Redirect stdout to client */ //line:netp:servedynamic:dup2
	Execve(filename, emptylist, environ); /* Run CGI program */ //line:netp:servedynamic:execve
    }
    Wait(NULL); /* Parent waits for and reaps child */ //line:netp:servedynamic:wait
}
/* $end serve_dynamic */

/*
 * clienterror - returns an error message to the client
 */
/* $begin clienterror */
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
/* $end clienterror */


void Log(const char *msg){
    FILE *fp;
    fp = fopen(log_path, "w+");
    //fprintf(fp, "This is testing for fprintf...");
    fputs(msg, fp);
    fclose(fp);
}