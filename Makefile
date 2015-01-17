#use g++ for everything
CC= g++  	

# include debugging symbols in object files,
# and enable all warnings
CXXFLAGS= -g -Wall

#include debugging symbols in executable
LDFLAGS= -g

server: server.o 
	g++ -o server server.o

server.o: server.c 

clean:
	$(RM) server *.o