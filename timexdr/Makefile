CC = gcc
CFLAGS = -Wall -W -Wshadow -Wstrict-prototypes -Wcast-qual -Wcast-align \
	 -Wwrite-strings -O2 -pipe -g3
SRC = timexdr.c
OBJ = $(SRC:.c=.o)
LIBS = -lusb
DOCS = AUTHORS CHANGES LICENSE
PREFIX = /usr/local
DESTBIN = $(PREFIX)/bin
DESTMAN = $(PREFIX)/man/man1

all: timexdr

timexdr: $(SRC:.c=.h) $(SRC)
	$(CC) $(CFLAGS) -o timexdr $(SRC) $(LIBS)

clean:
	rm -f $(OBJ) timexdr *~

install:
	install -m 755 -d $(DESTBIN)
	install -m 755 timexdr $(DESTBIN)
	install -m 755 -d $(DESTMAN)
	install -m 644 timexdr.1 $(DESTMAN)

uninstall:
	rm -f $(DESTBIN)/timexdr
	rm -f $(DESTMAN)/timexdr.1

