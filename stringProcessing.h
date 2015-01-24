#pragma once

extern int getCommand(char* commLine, Method* method, char* fname, HttpVersion *version);
extern FileType getFileType(char *fname);
extern int removeDotSegments(char source[]);