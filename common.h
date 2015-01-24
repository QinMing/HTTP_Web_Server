#pragma once

#include <stdio.h>
#include <stdlib.h>

#define RCVBUFSIZE 1440
#define MAXCOMMLEN 10
#define MAXFNAMELEN 256
#define MAXVERSIONLEN 32

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

static inline void error(const char* msg) {
    perror(msg);
    exit(-1);
}