CC=g++
CFLAGS=-I
CFLAGS+=-Wall
CPPPOSTFIX=.cpp
FILE=LogServer
LIBS=-lpthread

all: server

server: $(FILE)$(CPPPOSTFIX)
	$(CC) $(CFLAGS) $(FILE)$(CPPPOSTFIX) -o $(FILE) $(LIBS)

clean:
	rm -f *.o server