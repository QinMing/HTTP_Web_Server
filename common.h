#pragma once

#include <stdio.h>
#include <stdlib.h>

#define SENDSIZE 8192
#define RCVBUFSIZE 1440
#define MAXCOMMLEN 10
#define MAXFNAMELEN 256
#define MAXVERSIONLEN 32
#define MAXDOMAINLEN 256

#define WAITLONG 30
#define WAITSHORT 5

typedef struct {
    int major;
    int minor;
}HttpVersion;

typedef enum {
    html, jpg, jpeg, png, ico, other
} FileType;

typedef enum{
    GET
} Method;

typedef struct {
    int  restSize;
    int  unconfirmSize;
    char *tail;
    char *nextHead;
    char buff[RCVBUFSIZE+1];
}RecvBuff;

static inline void error(const char* msg) {
    perror(msg);
    exit(-1);
}

extern int isVerLower(HttpVersion ver);