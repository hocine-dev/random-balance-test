CC = gcc
CFLAGS = -Wall -pthread

# Cibles
all: client_program server_program

client_program: client.o
    $(CC) $(CFLAGS) -o client_program client.o

server_program: server.o
    $(CC) $(CFLAGS) -o server_program server.o

client.o: client.c
    $(CC) $(CFLAGS) -c client.c

server.o: server.c
    $(CC) $(CFLAGS) -c server.c

clean:
    rm -f *.o client_program server_program

.PHONY: all clean