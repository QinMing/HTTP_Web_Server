#pragma once

#define RCVBUFSIZE 1280
#define MAXCOMMLEN 10
#define MAXFNAMELEN 256
#define MAXVERSIONLEN 10
#define MAXDOMAINLEN 256

typedef struct {
    int major;
    int minor;
}HttpVersion;

typedef enum {
    html, jpg, jpeg, png, ico, other
} FileType;

static inline void error(const char* msg) {
    perror(msg);
    exit(-1);
}
