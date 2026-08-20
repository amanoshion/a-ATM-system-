CC 		= gcc
CFLAGS 		= -g -Wall
LDFLAGS 	= -lsqlite3 -lsodium

TARGETS 	= server.out client.out

.PHONY : all clean

all: $(TARGETS)

client.out: client.c
	$(CC) $(CFLAGS) client.c -o client.out
server.out: server.c db.c
	$(CC) $(CFLAGS) server.c db.c -o server.out $(LDFLAGS)
clean:
	rm -f $(TARGETS)
