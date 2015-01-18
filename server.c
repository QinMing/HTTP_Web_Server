#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define RCVBUFSIZE 128
#define MAXCOMMLEN 10
#define MAXFNAMELEN 256

void error(const char* msg) {
    perror(msg);
    exit(1);
}

void getCommand (char* commLine, char* &comm, char* &fname) {
    char temp;
    int ind = 0;
    comm = (char*) malloc(MAXCOMMLEN);
    fname = (char*) malloc(MAXFNAMELEN);
    temp = commLine[ind];
    while (ind < MAXCOMMLEN && temp != ' ') {
        comm[ind] = temp;
        temp = commLine[++ind];
    }
    if (ind == MAXCOMMLEN)
        printf("command length exceed\n");
    comm[ind] = '\0';
    int fInd = 0;
    temp = commLine[++ind];
    while (temp != ' ') {
        fname[fInd++] = temp;
        temp = commLine[++ind];
    }
    fname[fInd] = '\0';
}

int main(int argc, char* argv[]) {
    int sock, csock, portno, clilen, n;
    char rcvBuff[RCVBUFSIZE];
    int rcvMsgSize;
    struct sockaddr_in serv_addr, cli_addr;
    
	if (argc < 3) {
		error("ERROR: not enough argument. Expecting port number and root path");
	}
	chdir(argv[2]);

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
    char *comm, *fname;
    char defaultPage[] = "./index.html";
    FILE* fd;
    int fsize;
    char *content;
	char keyboardChar[128] = "\0";
    while (keyboardChar[0]!='q' && keyboardChar[0]!='Q') {
        if((csock = accept(sock, (struct sockaddr*) &cli_addr, &cliaddr_len)) < 0) 
            error("Accepct error");
        if (fork() == 0) {//!!!!!!!!!!csock might be changed due to synch problem--Ming
            if((rcvMsgSize = recv(csock, rcvBuff, RCVBUFSIZE, 0)) < 0)
                error("Receive error");
			printf("[Received]================\r\n");
            printf("%s\r\n", rcvBuff);
            getCommand(rcvBuff, comm, fname);
            if (fname[0] == '/') {//debug
                if ((fd = fopen(defaultPage, "r")) < 0) 
                    error("File open error");
                fseek(fd, 0, SEEK_END);  // set the position of fd in file end(SEEK_END)
                fsize = ftell(fd);       // return the fd current offset to beginning
                rewind(fd);              // reset fd to the beginning
                content = (char*) malloc(fsize+1); // for safety add 1
                if (fread(content, 1, fsize, fd) != fsize) 
                    error("Read file error");
                
				//printf("[Sending] == == == == ==\r\n");

                const char initLine[]="HTTP/1.0 200 OK\r\n";
				if (send(csock, initLine, sizeof(initLine), 0) != sizeof(initLine))
					error("Send Error");
				//printf("%s", initLine);

                const char contentType[]="Content-Type: text/html\r\n";
                if (send(csock, contentType, sizeof(contentType), 0) != sizeof(contentType))
                    error("Send Error");
				//printf("%s", contentType);


				char contentLength[20];
				sprintf(contentLength, "Content-Length: %d\r\n",fsize);
				if (send(csock, contentLength, sizeof(contentLength), 0) != sizeof(contentLength))
				    error("Send Error");
				//printf("%s", contentLength);

                if (send(csock, content, fsize, 0) != fsize)
                    error("Send error");
				//printf("%s", content);

			} else {
                printf("\r\n%s\r\n", fname);
            }
			//close(csock);
			exit(0);
        }
		close(csock);
		//scanf("%s", keyboardChar);
    }
	close(sock);
	exit(0);
}
