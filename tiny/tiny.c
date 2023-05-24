/* $begin tinymain */
/*
 * tiny.c - A simple, iterative HTTP/1.0 Web server that uses the
 *     GET method to serve static and dynamic content.
 *
 * Updated 11/2019 droh
 *   - Fixed sprintf() aliasing issue in serve_static(), and clienterror().
 */
#include "csapp.h"
void doit(int fd);
void read_requesthdrs(rio_t *rp);
int parse_uri(char *uri, char *filename, char *cgiargs);
void serve_static(int fd, char *filename, int filesize, char *method);
void get_filetype(char *filename, char *filetype);
void serve_dynamic(int fd, char *filename, char *cgiargs, char *method);
void clienterror(int fd, char *cause, char *errnum, char *shortmsg,
                 char *longmsg);
void echo (int connfd);
void sig_child_handeler(int sig);


int main(int argc, char **argv) {
  int listenfd, connfd;
  char hostname[MAXLINE], port[MAXLINE];
  socklen_t clientlen;
  struct sockaddr_storage clientaddr;

  /* Check command line args */
  if (argc != 2) {
    fprintf(stderr, "usage: %s <port>\n", argv[0]);
    exit(1);
  }

  listenfd = Open_listenfd(argv[1]);
  while (1) {
    clientlen = sizeof(clientaddr);
    connfd = Accept(listenfd, (SA *)&clientaddr,
                    &clientlen);  // line:netp:tiny:accept
    Getnameinfo((SA *)&clientaddr, clientlen, hostname, MAXLINE, port, MAXLINE,
                0);
    printf("Accepted connection from (%s, %s)\n", hostname, port);
    doit(connfd);   // line:netp:tiny:doit
    Close(connfd);  // line:netp:tiny:close
  }
}

void doit(int fd) // HTTP 트랜잭션을 처리하는 함수
{
  int is_static;
  struct stat sbuf;
  char buf[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE];
  char filename[MAXLINE], cgiargs[MAXLINE];
  rio_t rio;

  // 요청 라인을 읽고 분석하는 부분
  Rio_readinitb(&rio, fd); // rio를 초기화, fd와 rio 연결한다.
  Rio_readlineb(&rio, buf, MAXLINE); // new line을 만날때 까지, 바이트를 읽어서 buf에 저장, 그리고 rio에 저장
  printf("Request headers:\n");
  printf("%s", buf);
  sscanf(buf, "%s %s %s", method, uri, version); // buf에 담긴 string을 3의 문자열로 나눈다음 각각 담겠다는 뜻.

  // tiny 는 GET 요청만 받아서, 다른 요청이 들어오면 에러메세지 보내고 다시 메인으로 return.
  if(!(strcasecmp(method, "GET")== 0) && !(strcasecmp(method, "HEAD") == 0)) {
    clienterror(fd, method, "501", "Not implemented", "Tiny does not implement this method");
    return;
  }
  read_requesthdrs(&rio);

  is_static = parse_uri(uri, filename, cgiargs);

  if(stat(filename, &sbuf) < 0) { // stat() 함수는 filename 파일로 가서, sbuf에는 파일에 대한 정보를 가져옴, 만약 잘반환하면 0을 반환, 실패하면 -1 반환
    clienterror(fd, filename, "404", "Not found", "Tiny couldn't find this file");
    return;
  }
  if(is_static) { // 만약 is_static 이 정적 컨텐츠이면,
    if(!(S_ISREG(sbuf.st_mode)) || !(S_IRUSR & sbuf.st_mode)) { // 보통 파일인지, 읽기권한 있는지 확인 후 아니면 에러
      clienterror(fd, filename, "403", "Forbidden", "Tiny couldn't read the file");
      return;
    }
    serve_static(fd, filename, sbuf.st_size, method); // 보통파일이고, 읽기권한도 있으면, 
  }
  else { // 동적 컨텐츠라면
    if(!(S_ISREG(sbuf.st_mode)) || !(S_IXUSR & sbuf.st_mode)) { // 보통 파일이고, 실행 가능한지 확인,
      clienterror(fd, filename, "403", "Forbidden", "Tiny couldn't run the CGI program");
      return;
    }
    serve_dynamic(fd,filename,cgiargs,method);
  }

}

void clienterror(int fd, char *cause, char *errnum, char *shortmsg,char *longmsg)
{
  char buf[MAXLINE], body[MAXBUF];

    /* Build the HTTP response body */
  sprintf(body, "<html><title>Tiny Error</title>");
  sprintf(body, "%s<body bgcolor=""ffffff"">\r\n", body);
  sprintf(body, "%s %s : %s\r\n", body, errnum, shortmsg);
  sprintf(body, "%s <p> %s: %s\r\n", body, longmsg, cause);
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
  char buf[MAXLINE];
  Rio_readlineb(rp, buf, MAXLINE);
  while(strcmp(buf, "\r\n"))
  {
    Rio_readlineb(rp, buf, MAXLINE);
    printf("%s", buf);
  }
  return;
}

// URI 를 받아서 filename과 cgiargs 내용 넣기 
int parse_uri(char *uri, char *filename, char *cgiargs)
{
  char *ptr;
  // 정적 컨텐츠이면,
  if(!strstr(uri, "cgi-bin")) { // 2번째 인자로 주어진 문자열이 1번째에 있다면 시작하는 포인터를 반환하는 함수, 없으면 NULL 반환
    strcpy(cgiargs, ""); // 문자열 초기화
    strcpy(filename, "."); // '.'로 문자열 초기화
    strcat(filename, uri); // filename 뒤에 uri 붙이기
    if(uri[strlen(uri)-1] == '/'){ // uri의 마지막 문자가 '/' 인지 확인
      strcat(filename, "home.html"); // 마지막 문자가 / 이면 home.html을 filename에 넣어주기
      return 1;
    }
  }
  else { // 동적 컨텐츠이면,
    ptr = index(uri, '?'); //uri 에서 '?' 를 찾아서 그 위치를 가리키는 포인터 반환
    if(ptr){
      strcpy(cgiargs, ptr+1); // ptr+1 부터를 cgiargs 에 넣기
      *ptr = '\0'; // 시작값을 ? 에서 \O로 변경, 즉 문자열 나누기!
    }else {
      strcpy(cgiargs, ""); // 만약 ptr 값이 없으면.. ""로 초기화
    }
    strcpy(filename, "."); // filename '.' 으로 초기화
    strcat(filename, uri); // filename 에 uri 붙이기
    return 0;
  }
}

void serve_static(int fd, char *filename, int filesize, char *method)
{
  int srcfd;
  char *srcp, filetype[MAXLINE], buf[MAXLINE];

  get_filetype(filename, filetype);
  sprintf(buf, "HTTP/1.1 200 OK\r\n");
  sprintf(buf, "%sServer: Tiny Web Server\r\n", buf);
  sprintf(buf, "%sConnection: close\r\n", buf);
  sprintf(buf, "%sContent-length: %d\r\n", buf, filesize);
  sprintf(buf, "%sContent-type: %s\r\n\r\n", buf, filetype);
  Rio_writen(fd, buf, strlen(buf));
  printf("Response headers:\n");
  printf("%s", buf);

  if(strcasecmp(method, "GET") == 0) {
    srcfd = Open(filename, O_RDONLY, 0);
    // void *mmap(void *addr, size_t length, int prot, int flags, int fd, off_t offset);
    srcp = Mmap(0, filesize, PROT_READ, MAP_PRIVATE, srcfd, 0);
    Close(srcfd);
    Rio_writen(fd,srcp,filesize);
    // 할당된 메모리 영역 해제
    Munmap(srcp,filesize);
  }
}
void serve_dynamic(int fd, char *filename, char *cgiargs, char *method)
{
  char buf[MAXLINE], *emptylist [] = {NULL};

  sprintf(buf, "HTTP/1.1 200 OK\r\n");
  Rio_writen(fd, buf, strlen(buf));
  sprintf(buf, "Server: Tiny Web Server\r\n");
  Rio_writen(fd, buf, strlen(buf));

  if (Fork() == 0) { /* Child */
    /* Real server would set all CGI vars here */
    setenv("QUERY_STRING", cgiargs, 1); // 인자값을 환경변수에 세팅
    setenv("REQUEST_METHOD", method, 1);
    Dup2(fd, STDOUT_FILENO);   // fd 값을 STDOUT_FILENO 로 복사하고, fd closed.
    Execve(filename, emptylist, environ); /* Run CGI program */
}
    Signal(SIGCHLD, sig_child_handeler);
}
void sig_child_handeler(int sig){
  int old_error = errno;
  while(waitpid(-1, NULL, 0) > 0)
    errno = old_error;
}

void get_filetype(char *filename, char *filetype) 
{
  if (strstr(filename, ".html")){
    strcpy(filetype, "text/html");
  }
  else if (strstr(filename, ".gif")){
    strcpy(filetype, "image/gif");
  }
  else if (strstr(filename, ".png")){
    strcpy(filetype, "image/png");
  }
  else if (strstr(filename, ".jpg")){
    strcpy(filetype, "image/jpeg");
  }
  else if (strstr(filename, ".mp4")){
    strcpy(filetype, "video/mp4");
  }
  else{
    strcpy(filetype, "text/plain");
  }
}