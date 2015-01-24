#pragma once

extern int getCommand(const char* commLine, Method* method, char* fname, HttpVersion *version);
extern FileType getFileType(char *fname);
extern int removeDotSegments(char source[]);