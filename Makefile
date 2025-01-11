# Variables

CC = gcc

CFLAGS = -Wall -Wextra



# Cibles par défaut

all: s c



# Règle pour compiler et lier s.c en l'exécutable s

s: s.o

	$(CC) $(CFLAGS) s.o -o s -lm



# Règle pour compiler et lier c.c en l'exécutable c

c: c.o

	$(CC) $(CFLAGS) c.o -o c -lm



# Règle pour compiler s.c en s.o

s.o: s.c

	$(CC) $(CFLAGS) -c s.c -o s.o



# Règle pour compiler c.c en c.o

c.o: c.c

	$(CC) $(CFLAGS) -c c.c -o c.o



# Règle pour nettoyer les fichiers objets et exécutables

clean:

	rm -f *.o s c

