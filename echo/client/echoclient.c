#include "../csapp.h"

int main(int argc, char **argv){
    int clientfd;
    char *host, *port, buf[MAXLINE];
    rio_t rio;

    if(argc != 3){
        fprintf(stderr, "usage: %s <host> <port>\n", argv[0]);
        exit(0);
    }

    host = argv[1];
    port = argv[2];

    // ubuntu: 52184
    clientfd = Open_clientfd(host, port); // 2. socket(), connect() 서버 연결

    Rio_readinitb(&rio, clientfd); // 5. RIO 패키지를 사용하여 clientfd로부터 읽기 위한 버퍼를 초기화.
    while (Fgets(buf, MAXLINE, stdin) != NULL){ // 6. 사용자로 부터 입력 값 받고
        Rio_writen(clientfd, buf, strlen(buf)); // 7. 입력 값을 서버 전달
        Rio_readlineb(&rio, buf, MAXLINE);      // 9. 서버에서 보내준 값 읽기
        Fputs(buf, stdout);                     // 10. 읽은 값을 터미널에 보여주기

    }
    Close(clientfd);
    exit(0);
}