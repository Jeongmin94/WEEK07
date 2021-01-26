#include "csapp.h"

void doit(int fd);
void read_requesthdrs(rio_t *rp);
int parse_uri(char *uri, char *filename, char *cgiargs);
void serve_static(int fd, char *filename, int filesize, char *method);
void get_filetype(char *filename, char *filetype);
void serve_dynamic(int fd, char *filename, char *cgiargs);
void clienterror(int fd, char *cause, char *errnum, char *shortmsg, char *longmsg);
void echo(int connfd);

int main(int argc, char **argv)
{
    int listenfd, connfd;
    char hostname[MAXLINE], port[MAXLINE];
    socklen_t clientlen;
    struct sockaddr_storage clientaddr;
    
    if (argc != 2) {
        fprintf(stderr, "usage: %s <port>\n", argv[0]);
        exit(1);
    }
    listenfd = Open_listenfd(argv[1]);
    while (1) {
        clientlen = sizeof(clientaddr);
        connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen);
        Getnameinfo((SA *)&clientaddr, clientlen, hostname, MAXLINE,
                    port, MAXLINE, 0);
        printf("-------------listening of port %s-------------\n", argv[1]);
        printf("Accepted connection from (%s, %s)\n", hostname, port);
        doit(connfd);
        // echo(connfd);
        Close(connfd);
    }
}
void doit(int fd)
{
    int is_static;
    struct stat sbuf;
    char buf[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE];
    char filename[MAXLINE], cgiargs[MAXLINE];
    rio_t rio;

    Rio_readinitb(&rio, fd);
    Rio_readlineb(&rio, buf, MAXLINE);
    // 요청 헤더의 첫 라인만 버퍼에 저장
    // 버퍼에는 method, uri, version만 저장됨
    printf("Request headers:\n");
    printf("%s", buf);

    // 버퍼에서 method, uri, version을 분리해서 각각의 버퍼에 저장함
    sscanf(buf, "%s %s %s", method, uri, version);
    

    if(strcasecmp(method, "HEAD")==0 || strcasecmp(method, "GET")==0) {
        // 요청 헤더의 나머지 부분을 읽음
        read_requesthdrs(&rio);
    }
    else {
        if (strcasecmp(method, "HEAD")!=0 || strcasecmp(method, "GET")!=0) {
            clienterror(fd, method, "501", "Not implemented",
                        "Tiny does not implement this method");
            return;
        }
    }
    

    is_static = parse_uri(uri, filename, cgiargs);
    // uri를 파싱해서 filename, cgiargs 버퍼에 저장함
    // filename 에는 새롭게 만들어진 uri가 저장됨
    // cgiargs 에는 동적 콘텐츠 실행에 필요한 값들이 저장됨

    if (stat(filename, &sbuf) < 0) {
        clienterror(fd, filename, "404", "Not found",
                    "Tiny couldn't find this file");
        return;
    }

    // request header의 정보를 읽고 그에 맞는 작업을 수행
    // 그 뒤 처리한 작업에 대한 response header 출력
    if (is_static) { /* Serve static content */
        if (!(S_ISREG(sbuf.st_mode)) || !(S_IRUSR & sbuf.st_mode)) {
            clienterror(fd, filename, "403", "Forbidden",
                        "Tiny couldn't read the file");
            return;
        }
        serve_static(fd, filename, sbuf.st_size, method);
    }
    else { /* Serve dynamic content */
        if (!(S_ISREG(sbuf.st_mode)) || !(S_IXUSR & sbuf.st_mode)) {
            clienterror(fd, filename, "403", "Forbidden",   
                        "Tiny couldn't run the CGI program");
            return;
        }
        serve_dynamic(fd, filename, cgiargs);
    }
}
void clienterror(int fd, char *cause, char *errnum, char *shortmsg, char *longmsg)
{
    char buf[MAXLINE], body[MAXBUF];
    /* Build the HTTP response body */
    sprintf(body, "<html><title>Tiny Error</title>");
    sprintf(body, "%s<body bgcolor="
                  "ffffff"
                  ">\r\n",
            body);
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

void read_requesthdrs(rio_t *rp)
{
    // 클라이언트에 대한 정보나 fetch될 정보를 출력함
    printf("\r\n\r\nRead Request Headers\n");
    char buf[MAXLINE];
    Rio_readlineb(rp, buf, MAXLINE);
    while (strcmp(buf, "\r\n")) {
        Rio_readlineb(rp, buf, MAXLINE);
        printf("%s", buf);
    }
    return;
}
int parse_uri(char *uri, char *filename, char *cgiargs)
{
    printf("parsing uri %s\n", uri);
    char *ptr;
    if (!strstr(uri, "cgi-bin")) { /* Static content */
        strcpy(cgiargs, "");
        strcpy(filename, ".");
        strcat(filename, uri);
        if (uri[strlen(uri) - 1] == '/') strcat(filename, "home.html");
        else if (strstr(filename, "adder")) strcat(filename, ".html");
        printf("filename : %s\n\n", filename);
        return 1;
    }
    else { /* Dynamic content */
        ptr = index(uri, '?');
        if (ptr) {
            strcpy(cgiargs, ptr+1);
            *ptr = '\0';                    // 문자열 읽기 중단 지점 설정
        }
        else strcpy(cgiargs, "");
        strcpy(filename, ".");
        strcat(filename, uri);
        return 0;
    }
}

void serve_static(int fd, char *filename, int filesize, char *method)
{
    int srcfd;
    char *srcp, filetype[MAXLINE], buf[MAXBUF];

    get_filetype(filename, filetype);
    sprintf(buf, "HTTP/1.1 200 OK\r\n");
    sprintf(buf, "%sServer: Tiny Web Server\r\n", buf);
    sprintf(buf, "%sConnection: close\r\n", buf);
    sprintf(buf, "%sContent-length: %d\r\n", buf, filesize);
    sprintf(buf, "%sContent-type: %s\r\n\r\n", buf, filetype);
    Rio_writen(fd, buf, strlen(buf));

    if(!strcasecmp(method, "HEAD")) {
        printf("Static Response header:\n");
        printf("%s", buf);
        return;
    }
    // 위치 또는 서버에 대한 정보같은 응답에 대한 부가적인 정보 출력
    // 서버->클라이언트에서 전송해주는 개념
    printf("Static Response header:\n");
    printf("%s", buf);
    printf("Static filename : %s\n", filename);

    // 서버의 로컬 파일을 srcp에 저장하고, 이것을 소켓에 기록함
    // accept 소켓은 클라이언트 소켓과 통신 중인 상태이므로
    // 클라이언트는 accept 소켓에 기록된 로컬 파일의 내용을 브라우저로 읽음

    // 로컬 파일은 현재 서버 응용 프로그램에 실행중인 상태가 아니기 때문에
    // 로컬 파일을 열어 이것을 램에 할당시키고, 소켓에 기록할 수 있는 상태로 만들어 기록

    /* Send response body to client */
    srcfd = Open(filename, O_RDONLY, 0);
    srcp = Mmap(0, filesize, PROT_READ, MAP_PRIVATE, srcfd, 0);
    // mmap(st, len, prot, flags, fd, offset)

    // fd로 지정된 파일에서 offset에 해당하는 물리주소에서 시작하여 length 바이트 만큼을 start 주소로 대응시킨다. 
    // srcp = (char*)malloc(filesize);
    // Rio_readn(srcfd, srcp, filesize);

    Close(srcfd);
    Rio_writen(fd, srcp, filesize);
    Munmap(srcp, filesize);
    // free(srcp);
}
/*
 * get_filetype - Derive file type from filename
 */
void get_filetype(char *filename, char *filetype)
{
    if (strstr(filename, ".html"))
        strcpy(filetype, "text/html");
    else if (strstr(filename, ".gif"))
        strcpy(filetype, "image/gif");
    else if (strstr(filename, ".png"))
        strcpy(filetype, "image/png");
    else if (strstr(filename, ".jpg"))
        strcpy(filetype, "image/jpeg");
    else if (strstr(filename, ".mpeg"))
        strcpy(filetype, "video/mpeg");
    else if (strstr(filename, ".ogv"))
        strcpy(filetype, "video/ogv");
    else strcpy(filetype, "text/plain");
}

void serve_dynamic(int fd, char *filename, char *cgiargs)
{
    char buf[MAXLINE], *emptylist[] = {NULL};
    char filetype[MAXLINE];
    get_filetype(filename, filetype);
    /* Return first part of HTTP response */
    sprintf(buf, "HTTP/1.1 200 OK\r\n");
    sprintf(buf, "%sServer: Tiny Web Server\r\n", buf);
    sprintf(buf, "%sContent-type: %s\r\n", buf, filetype);
    Rio_writen(fd, buf, strlen(buf));
    

    setenv("QUERY_STRING", cgiargs, 1);
    char tmp[50] = getenv("QUERY_STRING");
    printf("QUERY_STRING : %s", tmp);

    printf("Dynamic Response Header\n");
    printf("%s", buf);
    printf("cgiargs %s\n", cgiargs);
    printf("filename %s\n", filename);
    if (Fork() == 0) { /* Child */
        /* Real server would set all CGI vars here */
        // QUERY_STRING(쿼리 파라미터라고도 부름)
        // GET 방식에서 URL 뒤에 나오는 정보를 저장하거나 폼입력 정보를 저장
        setenv("QUERY_STRING", cgiargs, 1);     // QUERY_STRING 환경 변수를 cgiargs로 덮어 씌움
        Dup2(fd, STDOUT_FILENO);                // 표준 출력 형식을 fd로 돌림
        Execve(filename, emptylist, environ);   // filename의 매개변수로 emptylist를 받고, 기존의 환경변수 설정을 사용해서 자식 프로그램 실행
    }
    // filename => .cig-bin/adder(실행 파일의 경로로 파싱됨)
    //              해당 경로에 있는 adder라는 프로그램을 실행
    // cgiargs => first=123&second=123
    Wait(NULL); /* Parent waits for and reaps child */
}

void echo(int connfd)
{
    printf("echo in\n");
    size_t n;
    char buf[MAXLINE];
    rio_t rio;

    Rio_readinitb(&rio, connfd);
    // n = Rio_readlineb(&rio, buf, MAXLINE);
    while((n=Rio_readlineb(&rio, buf, MAXLINE))!=0) {
        if(strcmp(buf, "\r\n")==0) break;
        printf("server received %d bytes\n", (int)n);
        // printf("server received %s\n", buf);
        Rio_writen(connfd, buf, n);
    }
    printf("echo out\n");
}