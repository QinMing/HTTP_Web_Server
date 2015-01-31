#pragma once

//=============================
//functions for RecvBuff struct
extern RecvBuff* newRecvBuff();

extern inline void deleteRecvBuff(RecvBuff * b);

//return 1: yes. "\r\n\r\n" is found
//return 0: no
//only b.unconfirmSize is set outside of here
extern int buffIsComplete(RecvBuff * b);

//this function can only be called after buffIsComplete() returns yes;
//It copy the string from *nextHead to the head of the buffer.
extern int buffChop(RecvBuff * b);
//=============================


extern int getCommand(char* commLine, Method* method, char* fname, HttpVersion *version);
extern FileType getFileType(char *fname);
extern int removeDotSegments(char source[]);
extern const char defaultPage[];
