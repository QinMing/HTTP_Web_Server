#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define RCVBUFSIZE 1280
#define MAXCOMMLEN 10
#define MAXFNAMELEN 256

void error(const char* msg) {
    perror(msg);
    exit(1);
}

int getDigitLen(int size) {
    int ret = 0;
    while (size) {
        ret ++;
        size /= 10;
    }
    return ret;
}

int isHTML(char *fname) {
    // TODO get last several char of file format
    if (fname[0] == '\0') // default page
        return 1;
    char temp;
    int ind = 0;
    temp = fname[0];   
    while (temp != '.')
        temp = fname[++ind];
    // if html
    if (fname[++ind] == 'h')
        return 1;
    else
        return 0;
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
    ind += 2;      // skip first '/' in order to get correct path
    if ((temp = commLine[ind]) == ' ') {
        fname[fInd] = '\0';
        return;
    }
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
    char *comm, *fname;
    char defaultPage[] = "html/index.html";
    FILE* fd;
    int fsize;
    int digLen, hdLen;
    char strTxtHeader[] = "HTTP/1.0 200 OK\r\nContent-Type: text/html\r\nContent-Length: ";
    char strImgHeader[] = "HTTP/1.0 200 OK\r\nContent-Type: image/png\r\nContent-Length: ";
    size_t headerLen;
    size_t len;
    int read_size, stat;
    while (1) {
        char *content, *header;
        if((csock = accept(sock, (struct sockaddr*) &cli_addr, &cliaddr_len)) < 0) 
            error("Accepct error");
        if (fork() == 0) {
            if((rcvMsgSize = recv(csock, rcvBuff, RCVBUFSIZE, 0)) < 0)
                error("Receive error");
            printf("%s", rcvBuff);
            getCommand(rcvBuff, comm, fname);

            if (fname[0] == '\0') {       // open default page
                if ((fd = fopen(defaultPage, "r")) < 0) 
                    error("File open error");
            }
            else {
                if (isHTML(fname) && (fd = fopen(fname, "r")) < 0)
                    error("File open error");
                if (!isHTML(fname) && (fd = fopen(fname, "rb")) < 0)
                    error("File open error");
            }

            fseek(fd, 0, SEEK_END);  // set the position of fd in file end(SEEK_END)
            fsize = ftell(fd);       // return the fd current offset to beginning
            rewind(fd);
            
            digLen = getDigitLen(fsize);
            hdLen = sizeof(strTxtHeader) + digLen + 3;   // two header are same length
            header = (char*) malloc(hdLen);
            if (isHTML(fname))
                snprintf(header, hdLen, "%s%d\n\n", strTxtHeader, fsize);
            else {
                snprintf(header, hdLen, "%s%d\n\n", strImgHeader, fsize);
                printf("---------------%s\n", header);
             }
            if (send(csock, header, hdLen, 0) != hdLen) 
                error("Send length error");
            
            content = (char*) malloc(fsize+1); 
            while (!feof(fd)) { 
                if (!isHTML(fname))
                    for (int i = 0; i < fsize; i++)
                        if (content[i] == '0')
                            printf("it happens\n");
                read_size = fread(content, 1, fsize-1, fd);
                printf("------- read_size is %d\n", read_size);
                
                stat = 0;
                do {
                    stat += write(csock, content, read_size);
                    printf("-------- send_size is %d\n", stat);
                } while (stat < read_size);
                bzero(content, sizeof(content));
            }
            /*
            content = (char*) malloc(fsize);
            if ( ( len = fread(content, 1, fsize, fd) ) != fsize) 
                error("Read file error");
            if (( len = write(csock, content, fsize)) != fsize)
                error("Send error");
            printf("write len:%d\n file size:%d\n", len, fsize);
            */

            free(header);
            free(content);
            bzero(rcvBuff, RCVBUFSIZE);
        }        
        close(csock);
    }
    return 1;
}
    
