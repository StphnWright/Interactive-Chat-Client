.PHONY: all clean
all: chatclient chatserver

chatserver: chatserver.c util.h
	gcc -g -Wall -Werror -pedantic-errors chatserver.c -o chatserver

chatclient: chatclient.c util.h
	gcc -g -Wall -Werror -pedantic-errors chatclient.c -o chatclient

clean:
	rm -f *.o chatserver chatserver.exe chatclient chatclient.exe
