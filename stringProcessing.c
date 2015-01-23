#include <string.h>
#include <stdlib.h> //for strtol (string to long), and/or atoi
#include "common.h"
#include "stringProcessing.h"


//TODO: check Complete packet

//static inline int notEndingCharacter(char c) {
//    return ( c != ' ' && c != '\t' && c != '\0' && c != '\r' && c != '\n' );
//}

//return -1 if there's 400 bad request
//version is like "1.1"
int getCommand(char* commLine, char* comm, char* fname, HttpVersion *version) {
    
    char verStr[MAXVERSIONLEN];
    comm[0] = '\0';
    fname[0] = '\0';
    verStr[0] = '\0';

    char *p = strtok(commLine, " \t\0\r\n");
    for (int i = 0; i < 3; ++i) {
        if (p == NULL)break;
        switch (i) {
        
        case 0:
            if (strlen(p) >= MAXCOMMLEN)
                return -1;
            strcpy(comm,p);
            break;
        
        case 1:
            if (strlen(p) >= MAXFNAMELEN)
                return -1;
            strcpy(fname, p);
            break;
        
        case 2:
            if (strncmp(p, "HTTP/",5) != 0)
                return -1;
            p += 5;
            if (strlen(p) >= MAXVERSIONLEN)
                return -1;
            strcpy(verStr, p);//E.g. "1.1"
            break;
        }
        p = strtok(NULL, " \t\0\r\n");
    }
    //TODO: print out commLine and see if it's changed

    if (fname[0] == '\0')//no need to check comm[0] == '\0'
        return -1;

    if (verStr[0] == '\0') {
        //if version is not specified, then it's a Simple-Request, e.g. HTTP/0.9
        //{0,0}means not specified
        version->major = 0;
        version->minor = 0;
    } else {
        for (p = verStr; *p != '.'; ++p) {
            if (*p<'0' || *p>'9') return -1;
        }
        version->major = atoi(verStr);
        ++p;
        char *minorStr = p;
        for (; *p != '\0'; ++p) {
            if (*p<'0' || *p>'9') return -1;
        }
        version->minor = atoi(minorStr);
    }
    return 0;



    //char temp;
    //int ind = 0;

    //temp = commLine[ind];
    //while (ind < MAXCOMMLEN && notEndingCharacter(temp)) {
    //    comm[ind] = temp;
    //    temp = commLine[++ind];
    //}
    //if (ind == MAXCOMMLEN) {
    //    printf("[Client Error] Command length exceeded\n");
    //}
    //comm[ind] = '\0';

    //int fInd = 0;
    //while (commLine[ind] == ' ' || commLine[ind] == '\t') {
    //    ++ind;
    //}
    //temp = commLine[ind];
    //while (notEndingCharacter(temp)) {
    //    fname[fInd++] = temp;
    //    temp = commLine[++ind];
    //}
    //fname[fInd] = '\0';
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
//Input:  URI from client
//Output: file path related to working directory in local file system.
//return: -1 bad path
//in accordance with <<URI Generic Syntax>> http://www.ietf.org/rfc/rfc3986.txt
//
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
    source[0]='.';//forming "./xxx"
    strcpy(source+1,dst);
    free(dst);
    return 0;
}
