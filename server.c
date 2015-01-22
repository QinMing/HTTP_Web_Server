#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <pthread.h>
#include <arpa/inet.h>

#define RCVBUFSIZE 1280
#define MAXCOMMLEN 10
#define MAXFNAMELEN 256
const char defaultPage[] = "index.html";

//TODO:
//400 error: HTTP/1.1 without host header. Or no colon, no value, etc.

int running = 1;

typedef enum {
    html, jpg, jpeg, png, ico, other
} FileType;

struct RespArg {
    int csock;
    char rcvBuff[RCVBUFSIZE];
    char comm[MAXCOMMLEN];
    char fname[MAXFNAMELEN];
    struct sockaddr_in cli_addr;
};

void error(const char* msg) {
    perror(msg);
    exit(1);
}

void* userIOSentry(void* sock) {
    printf("Waiting for request. To exit server, type in 'q + <Enter>'.\n");

    char key;
    do {
        key = getchar();
    } while (key != 'q' && key != 'Q');
    running = 0;
    close(*((int*)sock));
    return NULL;
}

static inline int notEndingCharacter(char c) {
    return ( c != ' ' && c != '\t' && c != '\0' && c != '\r' && c != '\n' );
}

void getCommand(char* commLine, char* comm, char* fname) {
    char temp;
    int ind = 0;

    temp = commLine[ind];
    while (ind < MAXCOMMLEN && notEndingCharacter(temp)) {
        comm[ind] = temp;
        temp = commLine[++ind];
    }
    if (ind == MAXCOMMLEN) {
        printf("[Client Error] Command length exceeded\n");
    }
    comm[ind] = '\0';

    fname[0] = '.';
    fname[1] = '/';
    int fInd = 2;
    while (commLine[ind] == ' ' || commLine[ind] == '\t') {
        ++ind;
    }
    if (commLine[ind] == '/') {
        ++ind;
    }
    temp = commLine[ind];
    while (notEndingCharacter(temp)) {
        fname[fInd++] = temp;
        temp = commLine[++ind];
    }
    fname[fInd] = '\0';
}


//Check the file type though its file name,
//Append default page name to fname if needed.
//
FileType checkFileType(char *fname) {
    //TODO : use strtok to check /../
    //TODO
    //Since HTTP/1.0 did not define any 1xx status codes, servers MUST NOT send a 1xx response to an HTTP/1.0 client except under experimental conditions.
    //http://www.w3.org/Protocols/rfc2616/rfc2616-sec10.html

    char *c = fname;
    char *tail;

    while (*c != '\0')
        ++c;

    tail = c;

    do {
        --c;
        if (*c == '/') {
            //no extension in file name
            //regard it as a path

            if (c + 1 == tail) {
                strcpy(tail, defaultPage);
            } else {
                *tail = '/';
                strcpy(tail + 1, defaultPage);
            }

            return html;
        }
    } while (*c != '.');

    ++c;
    if (strcmp(c, "html") == 0 ||
        strcmp(c, "HTML") == 0)
        return html;

    else if
        (strcmp(c, "jpg") == 0 ||
        strcmp(c, "JPG") == 0)
        return jpg;

    else if
        (strcmp(c, "jpeg") == 0 ||
        strcmp(c, "JPEG") == 0)
        return jpeg;

    else if
        (strcmp(c, "png") == 0 ||
        strcmp(c, "PNG") == 0)
        return png;

    else if
        (strcmp(c, "ico") == 0 ||
        strcmp(c, "ICO") == 0)
        return ico;

    else {
        printf("Warning: Undefined file extension\n");
        return other;
    }
}

int sendInitLine(int csock, int code) {
    char s[256] = "HTTP/1.1 ";

    const char str200[] = "200 OK\r\n";
    const char str404[] = "404 Not Found\r\n"
        "Content-Type: text/plain\r\n\r\nError 404 (Not Found).\r\n";
    const char str403[] = "403 Permission Denied\r\n"
        "Content-Type: text/plain\r\n\r\nError 403 (Permission Denied).\r\n";

    switch (code) {

    case 200:
        strcat(s, str200);
        break;

    case 404:
        strcat(s, str404);
        break;
    
    case 403:
        strcat(s, str403);
        break;

    default:
        printf("Error: Unimplemented response code\n");
        exit(-1);
        break;
    }
    int l = strlen(s);
    if (write(csock, s, l) != l)
        error("Error when sending");
    return 0;
}

static inline void sendEmptyLine(int csock) {
    if (write(csock, "\r\n", 2) != 2)
        error("Error when sending");
}

int sendHeader(int csock, FileType type, int fileSize) {
    char s[256] = "Content-Type: ";
    switch (type) {

    case html:
        strcat(s, "text/html\r\n");
        break;

    case jpg:
    case jpeg:
        strcat(s, "image/jpeg\r\n");
        //Seems like image/jpg is not a standard type
        //https://en.wikipedia.org/wiki/Internet_media_type#Type_image
        break;

    case png:
        strcat(s, "image/png\r\n");
        break;

    case ico:
        strcat(s, "image/x-icon\r\n");
        break;

    default:
        printf("Warning: Unimplemented file type\n");
        //exit(-1);
        s[0] = '\0';
        break;
    }
    sprintf(s, "%sContent-Length: %d\r\n\r\n", s, fileSize);
    //empty line is included

    int l = strlen(s);
    if (write(csock, s, l) != l)
        error("Error when sending");
    return 0;
}

int sendFile(int csock, char fname[]) {
    FILE* fd;
    int fsize;
    FileType type;

    //this will append the default page to fname if needed
    type = checkFileType(fname);

    //printf("[debug] file name: %s[end of debug]",fname);

    switch (type) {
    case html:
        fd = fopen(fname, "r");
        break;

    default:
        fd = fopen(fname, "rb");
        break;
    }
    //printf("debug, fd = %lld\n", (long long int)fd);
    if (fd == NULL) return -1;//send 404 later

    sendInitLine(csock, 200);

    fseek(fd, 0, SEEK_END);  // set the position of fd in file end(SEEK_END)
    fsize = ftell(fd);       // return the fd current offset to beginning
    rewind(fd);

    sendHeader(csock, type, fsize);

    char *content = (char*)malloc(fsize);
    if (( fread(content, 1, fsize, fd) ) != fsize)
        error("Read file error");
    if (( write(csock, content, fsize) ) != fsize)
        error("Send error");
    free(content);
    fclose(fd);
    return 0;
}

int checkAuth(struct sockaddr_in clientIP, char* filename) {
    /*  
    input: open file directory, client ip address
    output: 0 for deny, 1 for allow
    check ./htaccess whether the final directory is allowed to access by client
    //TODO if domain name in the ./htaccess file, should it be lookup dns
    */
    FILE *fd;
    char* line = NULL;
    size_t len = 0;
    ssize_t read;
    char comm[6];
    char *p = comm;
    char *pline;
    unsigned long binIPAddr, slideMask, reqIP;
    char strIPAddr[4];
    unsigned int offset, mask, i;
    char temp;
    reqIP = ntohl(clientIP.sin_addr.s_addr);
    if ((fd = fopen(filename, "r")) == NULL)
        error(".htaccess file open fail\n");
    while ((read = getline(&line, &len, fd) != -1)) {
        bzero(comm, sizeof(comm));
        pline = line;
        p = comm;
        while (*pline != ' ')
            *p++ = *pline++;
        pline++;
        while (*pline != ' ')
            pline++;
        temp = *(++pline); // judge this is ip or domain name
        if ( temp >= 65 && temp <= 90 || temp >= 97 && temp <= 122) {
            //printf("%s", pline);
            ;
        }
        else if (temp >= 48 && temp <= 57) {
            binIPAddr = 0;
            offset = 24;
            while (1) {
                bzero(strIPAddr, sizeof(strIPAddr));
                p = strIPAddr;
                while (*pline != '.' && *pline != '/' && *pline != '\n')
                    *p++ = *pline++;
                if (*pline == '\n') {
                    mask = atoi(strIPAddr);
                    break;
                }
                binIPAddr += ((atoi(strIPAddr)) << offset);
                offset -= 8;
                pline++;
            }
        } 

        slideMask = 1;
        slideMask = slideMask << (32 - mask);
        for (i = 0; i < mask; i++) {
            if ((reqIP & slideMask) != (binIPAddr & slideMask))
                break;
            slideMask = slideMask << 1;
        }
        if (i == mask) {
            if (strcmp(comm, "deny") == 0) {
                return 0;
            }
            else if (strcmp(comm, "allow") == 0) {
                return 1;
            }
        }   
    }

    return 1;
}

void* response(void* args) {
    int rcvMsgSize;
    struct RespArg *args_t;
    args_t = ( struct RespArg* ) args;
    
    unsigned int ip = args_t->cli_addr.sin_addr.s_addr;
    char ipClient[30];
    /*sprintf(ipClient, "%d.%d.%d.%d", ((ip >> 0) & 0xFF), ((ip >> 8) & 0xFF), ((ip >> 16) & 0xFF), ((ip >> 24) & 0xFF));
    printf("Client IP %s\n", ipClient);*/
    if (checkAuth(args_t->cli_addr,".htaccess") == 0) {
        sendInitLine(args_t->csock, 403);
        sendEmptyLine(args_t->csock);
        printf("debug, sending 403");
        close(args_t->csock);
        free(args_t);
        return NULL;
    }
     
    // Guarantees that thread resources are deallocated upon return
    pthread_detach(pthread_self());
    //Q: do we need to wait for the thread to finish, before the server exits by presing 'q' key ?

    /*
    use select() to try persistent connection
    do not close the socket until timeout of select
    */
    fd_set rdfds;
    //printf("I just want to know sizeof(rdfds):%d\n", sizeof(rdfds));
    struct timeval tv;
    int ret = 1;
    while (ret != 0) {
        FD_ZERO(&rdfds);
        FD_SET(args_t->csock, &rdfds);
        tv.tv_sec = 3;
        tv.tv_usec = 0;
        ret = select(args_t->csock + 1, &rdfds, NULL, NULL, &tv);
        //printf("select return value %d\n", ret);
        if (ret < 0)
            error("Select() error");
        else if (ret>0) {
            if (( rcvMsgSize = recv(args_t->csock, args_t->rcvBuff, RCVBUFSIZE, 0) ) < 0)
                error("Receive error");
            //TODO: recv might receive part of the packet, as in the book
            //accomplished by checking /r/n/r/n
            //and the head of a request might be in the last packet!
            printf("client socket: %d\n", args_t->csock);
            args_t->rcvBuff[rcvMsgSize] = '\0';
            //printf("%d\n", rcvMsgSize);
            //printf("[Received]====================\n%s\n", args_t->rcvBuff);
            getCommand(args_t->rcvBuff, args_t->comm, args_t->fname);
            printf("[Received]---------------\n%s %s (...)\n", args_t->comm, args_t->fname);
            if (strcmp("GET", args_t->comm) == 0) {
                if (sendFile(args_t->csock, args_t->fname) == -1) {
                    sendInitLine(args_t->csock, 404);
                    sendEmptyLine(args_t->csock);
                    //the status line is terminated by an empty line.
                    //see http://www.w3.org/Protocols/rfc2616/rfc2616-sec10.html
                    printf("debug, sending 404\n");
                    ret = 0;
                }
            }
        }
        if (ret == 0) {
            //printf("closed socket %d\n", args_t->csock);
            close(args_t->csock);
        }
    }
    free(args_t);
    return NULL;
}

int main(int argc, char* argv[]) {
    int sock, csock, portno;
    char rcvBuff[RCVBUFSIZE];
    char comm[MAXCOMMLEN];
    char fname[MAXFNAMELEN];

    struct sockaddr_in serv_addr, cli_addr;

    if (argc < 3) {
        error("ERROR: Not enough argument. Expecting port number and root path");
    }
    chdir(argv[2]);

    if (( sock = socket(AF_INET, SOCK_STREAM, 0) ) < 0)
        error("Sockfd is not available");

    bzero((char*)&serv_addr, sizeof(serv_addr));
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

    if (bind(sock, ( struct sockaddr* ) &serv_addr, sizeof(serv_addr)) < 0)
        error("Bind error");

    //set maximum # of waiting connections to 5
    if (listen(sock, 5) < 0)
        error("Listen error");

    pthread_t thread;
    pthread_create(&thread, NULL, userIOSentry, (void*)&sock);

    socklen_t cliaddr_len = sizeof(cli_addr);
    while (running) {
        if (( csock = accept(sock, ( struct sockaddr* ) &cli_addr, &cliaddr_len) ) < 0){
            if (running == 0){
                printf("Server exits normally.\n");
                break;
            }else{
                error("Accepct error");
            }
        }
        printf("master thread call one time **********\n");
        struct RespArg *args;
        args = malloc(sizeof(struct RespArg));
        args->csock = csock;
        args->cli_addr = cli_addr;
        pthread_create(&thread, NULL, response, (void *)args);
    }
    return 0;
}
