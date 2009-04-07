CC=gcc
CFLAGS=-g -Wall -pipe
LFLAGS=-lusb

HEADERS=*.h

# Comment these out if you want a more simple interface
CFLAGS+=-DUSE_READLINE
LFLAGS+=-lreadline

default: samba-script

%.o: %.c $(HEADERS)
	$(CC) -c -o $@ $< $(CFLAGS)

samba-script: at91sam_util.o at91sam.o commands.o nand.o nandecc.o sdramc.o pmc.o debug.o common.o boards.o
	$(CC) -o $@ $^ $(LFLAGS)

clean:
	rm -f *.o samba-script
