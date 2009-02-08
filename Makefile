CC=gcc
CFLAGS=-g -Wall -pipe
LFLAGS=-lusb

CFLAGS+=`pkg-config --cflags lua5.1`
LFLAGS+=`pkg-config --libs lua5.1`

default: samba-script

%.o: %.c
	$(CC) -c -o $@ $< $(CFLAGS)

samba-script: at91sam_util.o at91sam.o interpreter.o nand.o nandecc.o sdramc.o pmc.o debug.o common.o
	$(CC) -o $@ $^ $(LFLAGS)

clean:
	rm -f *.o samba-script
