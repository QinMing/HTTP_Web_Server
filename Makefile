#A makefile for CSE 124 project 1
CC= g++

# include debugging symbols in object files,
# and enable all warnings
CXXFLAGS= -g -Wall

#include debugging symbols in executable
LDFLAGS= -g

httpd: server.o 
	$(CC) -o httpd server.o

server.o: server.c 

clean:
	$(RM) server *.o