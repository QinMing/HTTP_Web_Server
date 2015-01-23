#pragma once

extern int getCommand(char* commLine, char* comm, char* fname, HttpVersion *version);
extern FileType getFileType(char *fname);
extern int removeDotSegments(char source[]);