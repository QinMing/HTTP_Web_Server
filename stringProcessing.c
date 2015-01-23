#include <string.h>
#include "stringProcessing.h"

const char defaultPage[] = "index.html";



static inline int notEndingCharacter(char c) {
    return ( c != ' ' && c != '\t' && c != '\0' && c != '\r' && c != '\n' );
}

void getCommand(char* commLine, char* comm, char* fname, char* version) {
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

//in accordance with <<URI Generic Syntax>> http://www.ietf.org/rfc/rfc3986.txt
//return -1: bad path
int removeDotSegments(char source[])
    char *src = source;

    //most website return 400 error if URI is not started with '/', according to our test with telnet.
    if (src[0] != '/') return -1;

    char *dst = malloc(strlen(src) + 1);
    dst[0] = '\0';

    while (src[0]) {
        if (strncmp(src, "/./", 3) == 0) {
            src += 2;
        } else if (strcmp(src, "/.") == 0) {
            src[1] = '\0';
        } else if (strncmp(src, "/../", 4) == 0 || strcmp(src, "/..") == 0) {
            char *p;

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
            char *p = strchr(src + 1, '/');
            if (p == NULL) 
                p = strchr(src, '\0');//to the end of src

            strncat(dst, src, p - src);
            src = p;
        }
    }
    strcpy(source,dst);
    free(dst);
    return 0;
}
