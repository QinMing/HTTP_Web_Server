#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define RCVBUFSIZE 128
#define MAXCOMMLEN 10

void error(const char* msg) {
    perror(msg);
    exit(1);
}

char* getCommand (char* commLine) {
    char *comm;
    char temp;
    int ind = 0;
    comm = (char*) malloc(MAXCOMMLEN);
    temp = commLine[ind];
    while (ind < MAXCOMMLEN && temp != ' ') {
        comm[ind] = temp;
        temp = commLine[++ind];
    }
    if (ind == MAXCOMMLEN)
        printf("command length exceed\n");
    comm[ind] = '\0';
    return comm;
}

int main(int argc, char* argv[]) {
    int sock, csock, portno, clilen, n;
    char rcvBuff[RCVBUFSIZE];
    int rcvMsgSize;
    struct sockaddr_in serv_addr, cli_addr;
    
    if (argc < 2) {
        error("ERROR, No Port Provided");
    }

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        error("Sockfd is not available");

    bzero((char* ) &serv_addr, sizeof(serv_addr));
    portno = atoi(argv[1]);
    /* 
    struct sockaddr_in have four fields
    sin_family: address family (AF_UNIX, AF_INET)
    sin_port: port number in network order
    in_addr: a struct contains s_addr the IP addr of sockaddr_in
    */ 
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;  // any packet send to sin_port will be accept, regardless of the dest ip addr
    serv_addr.sin_port = htons(portno);

    if (bind(sock, (struct sockaddr*) &serv_addr, sizeof(serv_addr)) < 0)
        error("Bind error");
    
    if (listen(sock, 128) < 0)
        error("Listen error");

    socklen_t cliaddr_len;
    cliaddr_len = sizeof(cli_addr);    
    char *comm;
    while (1) {
        if((csock = accept(sock, (struct sockaddr*) &cli_addr, &cliaddr_len)) < 0) 
            error("Accepct error");
        if((rcvMsgSize = recv(csock, rcvBuff, RCVBUFSIZE, 0)) < 0)
            error("Receive error");
        printf("%s", rcvBuff);
        comm = getCommand(rcvBuff);
        printf("\n%s\n", comm);
        close(csock);
    }
    return 1;
}
    
