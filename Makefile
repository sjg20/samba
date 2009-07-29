CC=gcc
CFLAGS=-g -Wall -pipe
LFLAGS=-lusb
OBJECTS=at91sam_util.o at91sam.o commands.o nand.o nandecc.o sdramc.o pmc.o debug.o common.o boards.o environment.o crc32.o 

# Comment these out if you don't want YAFFS2 image support
KDIR?=/home/andre/work/linux-2.6.20
YAFFSDIR=$(KDIR)/fs/yaffs2
CFLAGS+=-DCONFIG_YAFFS_UTIL
OBJECTS+=mkyaffs2image.o yaffs_packedtags2.o yaffs_tagsvalidity.o yaffs_ecc.o

HEADERS=*.h

CFLAGS+=-I$(KDIR)/include -I$(YAFFSDIR)

# Comment these out if you want a more simple interface
CFLAGS+=-DUSE_READLINE
LFLAGS+=-lreadline

default: samba-script

%.o: %.c $(HEADERS)
	$(CC) -c -o $@ $< $(CFLAGS)

yaffs_%.o: $(YAFFSDIR)/yaffs_%.c
	$(CC) -o $@ $< -c $(CFLAGS)

samba-script: $(OBJECTS)
	$(CC) -o $@ $^ $(LFLAGS)

clean:
	rm -f *.o samba-script
