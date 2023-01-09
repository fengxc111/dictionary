CC=gcc
CFLAGS=-g -Wall
LIBS=-lsqlite3

server: server.c
	${CC} ${CFLAGS} $^ -o server ${LIBS}

client: client.c
	${CC} ${CFLAGS} $^ -o client ${LIBS}

clean:
	rm -f server client