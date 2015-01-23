#pragma once

typedef enum {
    html, jpg, jpeg, png, ico, other
} FileType;

extern void getCommand(char* commLine, char* comm, char* fname, char* version);
extern FileType getFileType(char *fname);
extern int removeDotSegments(char source[]);