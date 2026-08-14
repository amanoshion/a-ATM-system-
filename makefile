CC 		= gcc
CFLAGS 		= -Wall
LDFLAGS 	= -lsqlite3

TARGETS 	= server.out client.out

.PHONY : all clean

all: $(TARGETS)

client.out: client.c
	$(CC) $(CFLAGS) client.c -o client.out
server.out: server.c
	$(CC) $(CFLAGS) server.c -o server.out $(LDFLAGS)
clean:
	rm -f $(TARGETS)
