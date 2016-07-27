#include "base.h"

void error(char *msg)
{
    fprintf(stderr, "%s: %s\n", msg, strerror(errno));
    exit(0);
}

pid_t Fork(void) 
{
    pid_t pid;

    if ((pid = fork()) < 0)
	error("Fork error");
    return pid;
}

void Execve(const char *filename, char *const argv[], char *const envp[]) 
{
    if (execve(filename, argv, envp) < 0)
	error("Execve error");
}

pid_t Wait(int *status) 
{
    pid_t pid;

    if ((pid  = wait(status)) < 0)
	error("Wait error");
    return pid;
}

int Open(const char *pathname, int flags, mode_t mode) 
{
    int rc;

    if ((rc = open(pathname, flags, mode))  < 0)
	error("Open error");
    return rc;
}

int Dup2(int fd1, int fd2) 
{
    int rc;

    if ((rc = dup2(fd1, fd2)) < 0)
	error("Dup2 error");
    return rc;
}


void *Mmap(void *addr, size_t len, int prot, int flags, int fd, off_t offset) 
{
    void *ptr;

    if ((ptr = mmap(addr, len, prot, flags, fd, offset)) == ((void *) -1))
	error("mmap error");
    return(ptr);
}

void Munmap(void *start, size_t length) 
{
    if (munmap(start, length) < 0)
	error("munmap error");
}

int Accept(int s, struct sockaddr *addr, socklen_t *addrlen) 
{
    int rc;

    if ((rc = accept(s, addr, addrlen)) < 0)
	error("Accept error");
    return rc;
}

ssize_t rio_readn(int fd, void *usrbuf, size_t n) 
{
    size_t nleft = n;
    ssize_t nread;
    char *bufp = usrbuf;

    while (nleft > 0) {
	if ((nread = read(fd, bufp, nleft)) < 0) {
	    if (errno == EINTR) /* Interrupted by sig handler return */
		nread = 0;      /* and call read() again */
	    else
		return -1;      /* errno set by read() */ 
	} 
	else if (nread == 0)
	    break;              /* EOF */
	nleft -= nread;
	bufp += nread;
    }
    return (n - nleft);         /* return >= 0 */
}

ssize_t rio_writen(int fd, void *usrbuf, size_t n) 
{
    size_t nleft = n;
    ssize_t nwritten;
    char *bufp = usrbuf;

    while (nleft > 0) {
	if ((nwritten = write(fd, bufp, nleft)) <= 0) {
	    if (errno == EINTR)  /* Interrupted by sig handler return */
		nwritten = 0;    /* and call write() again */
	    else
		return -1;       /* errno set by write() */
	}
	nleft -= nwritten;
	bufp += nwritten;
    }
    return n;
}

static ssize_t rio_read(rio_t *rp, char *usrbuf, size_t n)
{
    int cnt;

    while (rp->rio_cnt <= 0) {  /* Refill if buf is empty */
	rp->rio_cnt = read(rp->rio_fd, rp->rio_buf, 
			   sizeof(rp->rio_buf));
	if (rp->rio_cnt < 0) {
	    if (errno != EINTR) /* Interrupted by sig handler return */
		return -1;
	}
	else if (rp->rio_cnt == 0)  /* EOF */
	    return 0;
	else 
	    rp->rio_bufptr = rp->rio_buf; /* Reset buffer ptr */
    }

    /* Copy min(n, rp->rio_cnt) bytes from internal buf to user buf */
    cnt = n;          
    if (rp->rio_cnt < n)   
	cnt = rp->rio_cnt;
    memcpy(usrbuf, rp->rio_bufptr, cnt);
    rp->rio_bufptr += cnt;
    rp->rio_cnt -= cnt;
    return cnt;
}


void rio_readinitb(rio_t *rp, int fd) 
{
    rp->rio_fd = fd;  
    rp->rio_cnt = 0;  
    rp->rio_bufptr = rp->rio_buf;
}


ssize_t rio_readnb(rio_t *rp, void *usrbuf, size_t n) 
{
    size_t nleft = n;
    ssize_t nread;
    char *bufp = usrbuf;
    
    while (nleft > 0) {
	if ((nread = rio_read(rp, bufp, nleft)) < 0) 
            return -1;          /* errno set by read() */ 
	else if (nread == 0)
	    break;              /* EOF */
	nleft -= nread;
	bufp += nread;
    }
    return (n - nleft);         /* return >= 0 */
}

ssize_t rio_readlineb(rio_t *rp, void *usrbuf, size_t maxlen) 
{
    int n, rc;
    char c, *bufp = usrbuf;

    for (n = 1; n < maxlen; n++) { 
        if ((rc = rio_read(rp, &c, 1)) == 1) {
	    *bufp++ = c;
	    if (c == '\n') {
                n++;
     		break;
            }
	} else if (rc == 0) {
	    if (n == 1)
		return 0; /* EOF, no data read */
	    else
		break;    /* EOF, some data was read */
	} else
	    return -1;	  /* Error */
    }
    *bufp = 0;
    return n-1;
}

ssize_t Rio_readn(int fd, void *ptr, size_t nbytes) 
{
    ssize_t n;
  
    if ((n = rio_readn(fd, ptr, nbytes)) < 0)
	error("Rio_readn error");
    return n;
}

void Rio_writen(int fd, void *usrbuf, size_t n) 
{
    if (rio_writen(fd, usrbuf, n) != n)
	error("Rio_writen error");
}

/*void Rio_readinitb(rio_t *rp, int fd)
{
    rio_readinitb(rp, fd);
} */

ssize_t Rio_readnb(rio_t *rp, void *usrbuf, size_t n) 
{
    ssize_t rc;

    if ((rc = rio_readnb(rp, usrbuf, n)) < 0)
	error("Rio_readnb error");
    return rc;
}

ssize_t Rio_readlineb(rio_t *rp, void *usrbuf, size_t maxlen) 
{
    ssize_t rc;

    if ((rc = rio_readlineb(rp, usrbuf, maxlen)) < 0)
	error("Rio_readlineb error");
    return rc;
}

int open_clientfd(char *hostname, int port) 
{
    int clientfd;
    struct hostent *hp;
    struct sockaddr_in serveraddr;

    if ((clientfd = socket(AF_INET, SOCK_STREAM, 0)) < 0){
        return -1;
    }

    if ((hp = gethostbyname(hostname)) == NULL){
        return -2; 
    }
    bzero((char *) &serveraddr, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    bcopy((char *)hp->h_addr_list[0], (char *)&serveraddr.sin_addr.s_addr, hp->h_length);
    serveraddr.sin_port = htons(port);

    if (connect(clientfd, (struct sockaddr *) &serveraddr, sizeof(serveraddr)) < 0){
        return -1;
    }
    return clientfd;
}

int open_listenfd(int port) 
{
    int listenfd, optval=1;
    struct sockaddr_in serveraddr;
  
    // Create a socket descriptor
    if ((listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	return -1;
 
    // Eliminates "Address already in use" error from bind
    if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, 
		   (const void *)&optval , sizeof(int)) < 0)
	return -1;

    // Listenfd will be an endpoint for all requests to port on any IP address for this host
    bzero((char *) &serveraddr, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET; 
    serveraddr.sin_addr.s_addr = htonl(INADDR_ANY); 
    serveraddr.sin_port = htons((unsigned short)port); 
    if (bind(listenfd, (struct sockaddr *)&serveraddr, sizeof(serveraddr)) < 0)
	return -1;

    if (listen(listenfd, LISTENQ) < 0)
	return -1;
    return listenfd;
}


/*
*/
char* left_trim(char * stro, const char *stri){
    if( stri == NULL || stro == NULL || stri == stro ){
        error("invalid input in left_trim");
    }

    for (NULL; *stri != '\0' && isspace(*stri); ++stri){
      ;
    }
    return strcpy(stro, stri);
}
 
char* trim(char* stro, const char* stri){
    char *p = NULL;
    if( stri == NULL || stro == NULL){
        error("invalid input in trim");
    }
    
    left_trim(stro, stri);
    
    for(p = stro + strlen(stro) - 1;p >= stro && isspace(*p); --p){
      ;
    }

    *(++p) = '\0';
    return stro;
}
 
 
int get_conf(char *file, char *group, char *item, char *conf ){
    char group_name[MAXBUF],item_name[MAXBUF],*buf,*c,buf_i[MAXBUF], buf_o[MAXBUF];
    FILE *fp;
    int found=0;

    if( (fp=fopen( file,"r" ))==NULL ){
      printf( "open tws config file failed !\n");
      return(-1);
    }
    
    fseek( fp, 0, SEEK_SET );
    memset( group_name, 0, sizeof(group_name) );
    sprintf( group_name,"[%s]", group );
 
    while( !feof(fp) && fgets( buf_i, MAXBUF, fp )!=NULL ){
        left_trim(buf_o, buf_i);
        if( strlen(buf_o) <= 0 ){
            continue;
        }
        buf = NULL;
        buf = buf_o;
 
        if( found == 0 ){
            if( buf[0] != '[' ) {
                continue;
            } else if ( strncmp(buf,group_name,strlen(group_name))==0 ){
                found = 1;
                continue;
            }
        } else if( found == 1 ){
            if( buf[0] == '#' ){
                continue;
            } else if ( buf[0] == '[' ) {
                break;
            } else {
                if( (c = (char*)strchr(buf, '=')) == NULL ){
                    continue;
                }
                memset( item_name, 0, sizeof(item_name) );
                sscanf( buf, "%[^=|^ |^\t]", item_name );
                if( strcmp(item_name, item) == 0 ){
                    sscanf( ++c, "%[^\n]", conf );
                    char *KeyVal_o = (char *)malloc(strlen(conf) + 1);
                    if(KeyVal_o != NULL){
                      memset(KeyVal_o, 0, sizeof(KeyVal_o));
                      trim(KeyVal_o, conf);
                      if(KeyVal_o && strlen(KeyVal_o) > 0){
                        strcpy(conf, KeyVal_o);
                      }
                      free(KeyVal_o);
                      KeyVal_o = NULL;
                    }
                    found = 2;
                    break;
                } else {
                    continue;
                }
            }
        }
    }
    
    fclose( fp );
    if( found == 2 ){
      return(0);
    }
    else{
      return(-1);
    }
}

