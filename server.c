#include <unistd.h>
#include <sys/types.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <errno.h>
#include "common.h"
#include "permission.h"
#include "stringProcessing.h"

int running = 1;

struct RespArg {
    int csock;
    struct sockaddr_in cli_addr;
};

void* userIOSentry(void* sock) {
    printf("Waiting for request. To exit server, type in 'q + <Enter>'.\n");

    char key;
    do {
        key = getchar();
    } while (key != 'q' && key != 'Q');
    running = 0;
    close(*( (int*)sock ));
    //printf("see if this line in the thread can be reached.............");
    //A: Yes it can.
    return NULL;
}

//In this project, assume older version can only be exactly HTTP/1.0
int isVerLower(HttpVersion ver) {
    const HttpVersion myVer = {1,1};
    return ( ( ver.major < myVer.major ) || ( ver.major == myVer.major && ver.minor < myVer.minor ) );
}

int sendInitLine(int csock, int code, HttpVersion ver) {

    const char strVer10[] = "HTTP/1.0 ";
    const char strVer11[] = "HTTP/1.1 ";

    const char str200[] = "200 OK\r\n";

    const char str400[] = "400 Bad Request\r\n"
        "Connection: close\r\n"
        "Content-Type: text/plain\r\n\r\nError 400 (Bad Request).\r\n\r\n";

    const char str404[] = "404 Not Found\r\n"
        "Connection: close\r\n"
        "Content-Type: text/plain\r\n\r\nError 404 (Not Found).\r\n\r\n";

    const char str403[] = "403 Permission Denied\r\n"
        "Connection: close\r\n"
        "Content-Type: text/plain\r\n\r\nError 403 (Permission Denied).\r\n\r\n";

    //increase if necessary
    char s[1024];

    if (isVerLower(ver)) {
        strcat(s, strVer10);
    } else {
        strcat(s, strVer11);
    }

    switch (code) {

    case 200:
        strcat(s, str200);
        break;

    case 400:
        printf("debug, sending 400\n");
        strcat(s, str400);
        break;

    case 404:
        printf("debug, sending 404\n");
        strcat(s, str404);
        break;

    case 403:
        printf("debug, sending 403\n");
        strcat(s, str403);
        break;

    default:
        error("Error: Unimplemented response code\n");
        break;
    }
    int l = strlen(s);
    if (write(csock, s, l) != l)
        error("Error when sending");
    return 0;
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
        strcat(s, "application/octet-stream\r\n");
        break;
    }
    sprintf(s, "%sContent-Length: %d\r\n\r\n", s, fileSize);
    //empty line is included

    int l = strlen(s);
    if (write(csock, s, l) != l)
        error("Error when sending");
    return 0;
}

int sendFile(int csock, char fname[], HttpVersion ver) {
    FILE* fd;
    int fsize;
    FileType type;

    //this will append the default page to fname if needed
    type = getFileType(fname);

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

    sendInitLine(csock, 200, ver);

    fseek(fd, 0, SEEK_END);  // set the position of fd in file end(SEEK_END)
    fsize = ftell(fd);       // return the fd current offset to beginning
    rewind(fd);

    sendHeader(csock, type, fsize);

    //
    //Sending file in chunks if necessary
    //
    int chunkSize;
    if (fsize > SENDSIZE) {
        chunkSize = SENDSIZE;
    } else {
        chunkSize = fsize;
    }
    char *content = (char*)malloc(chunkSize);
    while (fsize > 0) {
        if (fsize < SENDSIZE) {
            chunkSize = fsize;
        }
        if (( fread(content, 1, chunkSize, fd) ) != chunkSize)
            error("Read file error");
        if (( write(csock, content, chunkSize) ) != chunkSize)
            error("Send error");
        fsize -= chunkSize;
    }

    free(content);
    fclose(fd);
    return 0;
}

//TODO Chunked transfer

//Only deal with GET. Assume client will not send body, but only initial line and headers.
//return -1 if csock need to be close. Maybe client error or HTTP/1.0
//return 0 the request is complete, and successfully responsed
//return 1 the request is not complete, still waiting.
int responseRequest(int csock, RecvBuff* recvBuff, struct sockaddr_in *cli_addr) {
    char fname[MAXFNAMELEN];
    HttpVersion version;
    Method method;

    if (( recvBuff->unconfirmSize =
        recv(csock, recvBuff->tail, recvBuff->restSize, 0) ) < 0)
        error("Receive error");
    if (!buffInspect(recvBuff)) return 1;

    printf("client socket: %d\n", csock);
    //rcvBuff[rcvMsgSize] = '\0'; !!careful!
    if (getCommand(recvBuff->buff, &method, fname, &version) == -1) {
        sendInitLine(csock, 400, version);
        return -1;
    }    
    if (method == GET) {
        printf("[Receive request]path=%s\n", fname);
        if (removeDotSegments(fname) == -1) {
            sendInitLine(csock, 400, version);
            return -1;
        }

        //TODO get directory of htaccess after it has been checked to be correctly
        char* htaccPath = getHtaccessPath(fname);
        if (checkAuth(*cli_addr, htaccPath) == 0) {
            sendInitLine(csock, 403, version);
            free(htaccPath);
            return -1;
        }
        free(htaccPath);

        if (sendFile(csock, fname, version) == -1) {
            sendInitLine(csock, 404, version);
            return -1;
        }
    }
    return buffChop(recvBuff);
    //buffChop return 1: still has data
}

void* threadMain(void* args) {
    // Guarantees that thread resources are deallocated upon return
    int errno;
    errno = pthread_detach(pthread_self());
    if (errno == EINVAL)
        error("Thread is not joinable  thread");
    if (errno == ESRCH)
        error("Thread cannot be found");

    int csock = ( ( struct RespArg* )args )->csock;
    struct sockaddr_in cli_addr = ( ( struct RespArg* )args )->cli_addr;
    free(( struct RespArg* )args);

    RecvBuff *recvBuff = newRecvBuff();//remember deleteRecvBuff()
    int ret = 1;
    int timeout = WAITLONG;
    fd_set rdfds;
    //printf("I just want to know sizeof(rdfds):%d\n", sizeof(rdfds));
    struct timeval tv;
    //use select() to try persistent connection
    //do not close the socket until timeout of select
    do{
        FD_ZERO(&rdfds);
        FD_SET(csock, &rdfds);
        tv.tv_sec = timeout;
        tv.tv_usec = 0;
        ret = select(csock + 1, &rdfds, NULL, NULL, &tv);
        if (ret > 0) {
            switch (responseRequest(csock, recvBuff, &cli_addr)) {
            case 1://expecting the rest of the request
                timeout = WAITLONG;
                break;

            case 0://response complete
                timeout = WAITSHORT;
                break;

            case -1://client error or HTTP/1.0, close connection
                ret = 0;
                break;
            }
        } else if (ret < 0) {
            error("Select() error");
        }
    } while (ret != 0);
    //printf("closed socket %d\n", csock);
    close(csock);
    deleteRecvBuff(recvBuff);
    return NULL;
}

int main(int argc, char* argv[]) {
    int sock, csock, portno;

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
        if (( csock = accept(sock, ( struct sockaddr* ) &cli_addr, &cliaddr_len) ) < 0) {
            if (running == 0) {
                break;
            } else {
                error("Accepct error");
            }
        }
        struct RespArg *args;
        args = malloc(sizeof(struct RespArg));
        args->csock = csock;
        args->cli_addr = cli_addr;
        pthread_create(&thread, NULL, threadMain, (void *)args);
    }
    printf("Server exits normally.\n");
    return 0;
}
