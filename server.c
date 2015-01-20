#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <pthread.h>

#define RCVBUFSIZE 1280
#define MAXCOMMLEN 10
#define MAXFNAMELEN 256
const char defaultPage[] = "index.html";
//The server is set to "HTTP/1.1", in function sendInitLine()

typedef enum {
    html, jpg, jpeg, png, ico, other
} FileType;

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
            //TODO : ask TA or professor

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
    //TODO
    //Since HTTP/1.0 did not define any 1xx status codes, servers MUST NOT send a 1xx response to an HTTP/1.0 client except under experimental conditions.
    //http://www.w3.org/Protocols/rfc2616/rfc2616-sec10.html


    const char str200[] = "200 OK\r\n";
    //const char str404[]="404 Not Found\r\n";
    const char str404[] = "404 Not Found\r\n"
        "Content-Type: text/plain\r\n\r\nError 404 (Not Found).\r\n";

    switch (code) {

    case 200:
        strcat(s, str200);
        break;

    case 404:
        strcat(s, str404);
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

void response(void* args) {
    int rcvMsgSize;
    struct RespArg *args_t;
    args_t = ( struct RespArg* ) args;

    // Guarantees that thread resources are deallocated upon return
    pthread_detach(pthread_self());

    /*
    use select() to try persistent connection
    do not close the socket until timeout of select
    */
    // read fd set initial
    fd_set rdfds;
    FD_ZERO(&rdfds);
    FD_SET(args_t->csock, &rdfds);
    struct timeval tv;
    tv.tv_sec = 5;
    tv.tv_usec = 0;
    int ret = 1;
    while (ret != 0) {
        ret = select(args_t->csock + 1, &rdfds, NULL, NULL, &tv);
        printf("time %ld sec %ld usec\n", tv.tv_sec, tv.tv_usec);
        //printf("select return value %d\n", ret);
        if (ret < 0)
            error("Select error");
        else if (ret>0) {
            if (( rcvMsgSize = recv(args_t->csock, args_t->rcvBuff, RCVBUFSIZE, 0) ) < 0)
                error("Receive error");
            printf("client socket: %d\n", args_t->csock);
            args_t->rcvBuff[rcvMsgSize] = '\0';
            printf("%d\n", rcvMsgSize);
            printf("[Received]====================\n%s\n", args_t->rcvBuff);
            getCommand(args_t->rcvBuff, args_t->comm, args_t->fname);
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

    if (listen(sock, 128) < 0)
        error("Listen error");

    socklen_t cliaddr_len = sizeof(cli_addr);
    while (1) {
        if (( csock = accept(sock, ( struct sockaddr* ) &cli_addr, &cliaddr_len) ) < 0)
            error("Accepct error");
        printf("master thread call one time **********\n");
        struct RespArg *args;
        args = malloc(sizeof(struct RespArg));
        args->csock = csock;
        pthread_t* thread;
        //thread = malloc(sizeof(pthread_t));
        pthread_create(thread, NULL, (void *)&response, (void *)args);
    }
    return 1;
}
