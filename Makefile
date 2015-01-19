#A makefile for CSE 124 project 1
CC= gcc
CompileFlag = -pthread

# include debugging symbols in object files,
# and enable all warnings
CFLAGS= -g -Wall
CXXFLAGS= -g -Wall

#include debugging symbols in executable
LDFLAGS= -g

httpd: server.o
	@$(CC) -o httpd server.o
	@echo Make complete.

server.o: server.c 
	@$(CC) $(CompileFlag) -c -o server.o server.c

clean:
	$(RM) httpd *.o *~
