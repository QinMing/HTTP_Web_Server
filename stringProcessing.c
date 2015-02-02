#include <string.h>
#include <stdlib.h> //for strtol (string to long), and/or atoi
#include "common.h"

const char defaultPage[] = "index.html";
char carriage[5] = "\r\n\r\n";

RecvBuff* newRecvBuff() {
    RecvBuff* b = malloc(sizeof(RecvBuff));
    b->restSize = RCVBUFSIZE;
    b->unconfirmSize = 0;
    b->tail = b->buff;
    b->nextHead = NULL;
    b->ptrEnd = carriage;
    b->startMatch = 0;
    return b;
}

void deleteRecvBuff(RecvBuff * b) {
    free(b);
}

//This function will check all unconfirmed data in the
//buffer, and change the pointers and sizes.
//only b->unconfirmSize is set outside of here
//Note: "\n\n" is not supported. A request must contain "\r\n\r\n".
//
//return 1: yes. "\r\n\r\n" is found
//return 0: no
int buffInspect(RecvBuff * b) {
    char *newtail = b->tail + b->unconfirmSize;
    printf("unconfirmSize %d\n", b->unconfirmSize);
        
    char *it = b->tail;
    b->nextHead = NULL;
    for (; it != newtail; ++it) {
        if (*it == *(b->ptrEnd)) {
            b->ptrEnd++;
            if (b->startMatch == 0)
                b->startMatch = 1;
        }
        else {
            if (b->startMatch == 1) {
                b->ptrEnd = carriage;
                b->startMatch = 0;
            }
        }
    }
    b->restSize -= b->unconfirmSize;
    b->unconfirmSize = 0;
    b->tail = newtail;
    if (*b->ptrEnd == '\0') {
        b->nextHead = b->tail;
        return 1;   
    }
    else
        return 0;
}

//this function can only be called after buffIsComplete() returns yes;
//It copy the string from *nextHead to the head of the buffer.
//return 1: still has data
//return 0: buffer is empty
int buffChop(RecvBuff * b) {
    char *src = b->nextHead;
    char *dst = b->buff;
    for (; src != b->tail; ++dst, ++src) {
        *dst = *src;
    }
    b->restSize += b->nextHead - b->buff;
    b->tail = dst;
    b->nextHead = NULL;

    return (b->tail != b->buff);
}

//string needs to be delete outside
char* getHtaccessPath(char fname[]) {
    char *tail = strrchr(fname, '/');
    char *head = malloc(MAXFNAMELEN);
    char *dst,*src;

    dst = head;
    src = fname;
    while (src != tail) {
        *(dst++) = *(src++);
    }
    strcpy(dst, "/.htaccess");
    return head;
}

static inline int notEndingCharacter(char c) {
    return ( c != ' ' && c != '\t' && c != '\0' && c != '\r' && c != '\n' );
}

static inline void nextToken(char **ptr) {
    while (**ptr == ' ' || **ptr == '\t') {
        ++( *ptr );
    }
}

//return -1 if there's 400 bad request
//version is like "1.1"
//Input: first line of request
//Output: fname is a raw URI
//NOTE: careful when changing this function. CommLine is not null-ending.
//A request must contain "\r\n\r\n" when checking the host header
int getCommand(char* commLine, Method* method, char* fname, HttpVersion *version) {
    char *ptr;
    int i;

    //get the method
    ptr = commLine;
    nextToken(&ptr);
    if (strncmp(ptr, "GET", 3) == 0) {
        *method = GET;
        ptr += 3;
    } else {
        printf("not GET command\n");
        return -1;
    }

    //get the raw URI
    nextToken(&ptr);
    i = 0;
    while (notEndingCharacter(*ptr)) {
        fname[i++] = *( ptr++ );
        if (i >= MAXFNAMELEN) return -1;
    }
    fname[i] = '\0';

    //get the HTTP version
    nextToken(&ptr);
    if (strncmp(ptr, "HTTP/", 5) == 0) {
        ptr += 5;
        char *head = ptr;
        if (*head<'0' || *head>'9') return -1;
        i = 0;
        for (; *ptr != '.'; ++ptr) {
            if (*ptr<'0' || *ptr>'9') return -1;
            if (++i > MAXVERSIONLEN) return -1;
        }
        *ptr = '\0';
        version->major = atoi(head);
        *ptr = '.';
        ++ptr;
        head = ptr;
        if (*head<'0' || *head>'9') return -1;
        for (; ( *ptr >= '0' && *ptr <= '9' ); ++ptr) {
            if (++i > MAXVERSIONLEN) return -1;
        }
        char temp = *ptr;
        *ptr = '\0';
        version->minor = atoi(head);
        *ptr = temp;
    } else {
        //if version is not specified, then it's a Simple-Request, e.g. HTTP/0.9
        //{0,0}means not specified
        version->major = 0;
        version->minor = 0;
    }

    //should reach end of line
    nextToken(&ptr);
    if (!(strncmp(ptr, "\r\n", 2) == 0 || *ptr == '\n'))
        return -1;

    //check if there is Host header, if HTTP/1.1 or above
    if (isVerLower(*version)) {
        return 0;
    } else {
        ++ptr;
        if (*ptr == '\n') ++ptr;
        while (strncmp(ptr, "\r\n\r\n", 4) != 0) {
            if (strncmp(ptr, "Host", 4) == 0)return 0;
            if (strncmp(ptr, "host", 4) == 0)return 0;
            if (strncmp(ptr, "HOST", 4) == 0)return 0;
            ++ptr;
        }
        return -1;
    }
}

//Check the file type though its file name,
//Append default page name to fname if needed.
//
FileType getFileType(char *fname) {
    //TODO : protect internal files such as .htaccess
    //TODO
    //Since HTTP/1.0 did not define any 1xx status codes, servers MUST NOT send a 1xx response to an HTTP/1.0 client except under experimental conditions.
    //http://www.w3.org/Protocols/rfc2616/rfc2616-sec10.html

    char *c = fname;
    while (*c != '\0')
        ++c;
    //now *c=='\0'

    if (*( c - 1 ) == '/') {
        strcpy(c, defaultPage);
        return html;
    }

    do {
        --c;
        if (*c == '/') {
            return other;
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

//This function does more than removing dot segments.
//Input:  raw URI, the fname from getCommand().
//Output: file path related to working directory in local file system.
//return: -1 bad path
//in accordance with <<URI Generic Syntax>> http://www.ietf.org/rfc/rfc3986.txt
//TODO: Call it
int removeDotSegments(char source[]) {
    //most website return 400 error if URI is not started with '/', according to our test with telnet.
    if (source[0] != '/') return -1;

    char *p;
    char *src = source;
    char *dst = malloc(strlen(src) + 1);
    dst[0] = '\0';

    while (src[0]) {
        if (strncmp(src, "/./", 3) == 0) {
            src += 2;
        } else if (strcmp(src, "/.") == 0) {
            src[1] = '\0';
        } else if (strncmp(src, "/../", 4) == 0 || strcmp(src, "/..") == 0) {

            if (src[3] == '\0') {
                //later
                src += 2;
                src[0] = '/';
            } else {
                //former
                src += 3;
            }

            p = strrchr(dst, '/');
            if (p) {
                *p = '\0';
            } else {
                free(dst);
                return -1;
            }
        } else {
            p = strchr(src + 1, '/');
            if (p == NULL)
                p = strchr(src, '\0');//to the end of src

            strncat(dst, src, p - src);
            src = p;
        }
    }
    source[0] = '.';//forming "./xxx"
    strcpy(source + 1, dst);
    free(dst);
    return 0;
}
