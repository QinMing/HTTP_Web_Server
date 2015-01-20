#A makefile for CSE 124 project 1
CC= gcc
CompileFlagHide = -pthread
CompileFlag =  
#I found that it still compile without -pthread

# include debugging symbols in object files,
# and enable all warnings
CFLAGS= -g -Wall
CXXFLAGS= -g -Wall

#include debugging symbols in executable
LDFLAGS= -g

httpd: server.o
	@$(CC) $(CompileFlag) -o httpd server.o
	@echo Make complete.

server.o: server.c 
	@$(CC) $(CompileFlag) -c -o server.o server.c

clean:
	$(RM) httpd *.o *~
