#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>

#define RCVBUFSIZE 1280
#define MAXCOMMLEN 10
#define MAXFNAMELEN 256
const char defaultPage[] = "html/index.html";

typedef enum {html,jpg,png} FileType ;

struct RespArg {
    int csock;
    char rcvBuff[RCVBUFSIZE];
    char comm[MAXCOMMLEN];
    char fname[MAXFNAMELEN];
};

void error(const char* msg) {
    perror(msg);
    exit(1);
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

void getCommand (char* commLine, char* comm, char* fname) {
    char temp;
    int ind = 0;
    temp = commLine[ind];
    while (ind < MAXCOMMLEN && temp != '\0' && temp != ' ') {
        comm[ind] = temp;
        temp = commLine[++ind];
    }
    if (ind == MAXCOMMLEN) {
        
        printf("command length exceed\n");
    }
    comm[ind] = '\0';
    int fInd = 0;
    while (commLine[ind]==' ') {
        ++ind;
    }
    if (commLine[ind]=='/') {
        ++ind;
    }
    temp = commLine[ind];
    while (temp != '\0' && temp != ' ') {
        fname[fInd++] = temp;
        temp = commLine[++ind];
    }
    fname[fInd] = '\0';
    printf("debug: %s\n",fname);
}

int sendInitLine(int csock, int code){
    const char strOK[]="HTTP/1.0 200 OK\r\n";
    const char* s;
    switch (code) {
        case 200:
            s=strOK;
            break;

        default:
            printf("Error: Unimplemented init line");
            exit(-1);
            break;
    }
    int l=strlen(s);
    if ( write(csock,s,l) != l)
        error("Error when sending");
    return 0;
}

int sendHeader(int csock, FileType type, int fileSize){
    char s[256]="Content-Type: ";
    switch (type) {
        case html:
            strcat(s,"text/html\r\n");
            break;
        case jpg:
            strcat(s,"image/jpg\r\n");
            break;
        case png:
            strcat(s,"image/png\r\n");
            break;
        default:
            printf("Error: Unimplemented file type");
            exit(-1);
            break;
    }
    sprintf(s,"%sContent-Length: %d\r\n\r\n",s,fileSize);
    int l=strlen(s);
    if ( write(csock,s,l) != l)
        error("Error when sending");
    return 0;
}

int sendFile(int csock,char fname[]){
    FILE* fd;
    int fsize;
    FileType type;

    if (fname[0] == '\0') {       // open default page
        fd = fopen(defaultPage, "r");
        type = html;
    } else {
        if (isHTML(fname)){
            type = html;
            fd = fopen(fname, "r" );
        }else{
            type = png;//debug  TODO
            fd = fopen(fname, "rb");
        }
    }
    if (fd<0) error("File open error");
    
    fseek(fd, 0, SEEK_END);  // set the position of fd in file end(SEEK_END)
    fsize = ftell(fd);       // return the fd current offset to beginning
    rewind(fd);
    
    sendHeader(csock,type,fsize);
    
    char *content = (char*) malloc(fsize);
    if (( fread(content, 1, fsize, fd)) != fsize)
        error("Read file error");
    if (( write(csock, content, fsize)) != fsize)
        error("Send error");
    free(content);
    fclose(fd);
    return 0;
}

void* response( void* args) {
    int rcvMsgSize;
    struct RespArg *args_t ;
    args_t = (struct RespArg*) args;
    if((rcvMsgSize = recv(args_t->csock, args_t->rcvBuff, RCVBUFSIZE, 0)) < 0)
        error("Receive error");
    args_t->rcvBuff[rcvMsgSize]='\0';
    printf("%d\n", rcvMsgSize);
    printf("[Received]====================\n%s\n", args_t->rcvBuff);
    getCommand(args_t->rcvBuff, args_t->comm, args_t->fname);
    if (strcmp("GET", args_t->comm) == 0) {
        sendInitLine(args_t->csock,200);
        sendFile(args_t->csock, args_t->fname);
    }
    close(args_t->csock);
    free(args_t);
}

int main(int argc, char* argv[]) {
    int sock, csock, portno, clilen, n;
    char rcvBuff[RCVBUFSIZE];
    char comm[MAXCOMMLEN];
    char fname[MAXFNAMELEN];
    
    //int rcvMsgSize;
    struct sockaddr_in serv_addr, cli_addr;
    
    if (argc < 3) {
        error("ERROR: Not enough argument. Expecting port number and root path");
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
    while (1) {
        if((csock = accept(sock, (struct sockaddr*) &cli_addr, &cliaddr_len)) < 0) 
            error("Accepct error");
        struct RespArg *args;
        args = malloc(sizeof(struct RespArg));
        args->csock = csock;
        pthread_t* thread;    
        thread = malloc(sizeof(pthread_t));
        pthread_create(thread, NULL, (void *)&response, (void *)args);
    }
    return 1;
}
