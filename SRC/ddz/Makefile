include ../makedefs

CFLAGS_DDRZ=-DDDRZ
CFLAGS_DDSZ=-DDDSZ
IFLAGS=-I./ -I../ -I../lib -I../main
OPTLFLAGS=-L../lib
LIBS=-lddcommon
CFLAGS=$(WARNFLAGS) $(OPTCFLAGS) $(IFLAGS)
LFLAGS=$(OPTLFLAGS)

all: common ddrz ddsz 

common:
	$(CC) $(CFLAGS) $(CDEFS) -o crctab.o -c crctab.c

ddrz:
	$(CC) $(CFLAGS) $(CFLAGS_DDRZ) $(CDEFS) -o ddrz-rbsb.o -c rbsb.c
	$(CC) $(CFLAGS) $(CFLAGS_DDRZ) $(CDEFS) -o ddrz-timing.o -c timing.c
	$(CC) $(CFLAGS) $(CFLAGS_DDRZ) $(CDEFS) -o ddrz-zm.o -c zm.c
	$(CC) $(CFLAGS) $(CFLAGS_DDRZ) $(CDEFS) -o ddrz.o -c ddrz.c	
	$(CC) $(LFLAGS) -o ddrz ddrz-rbsb.o ddrz-timing.o ddrz-zm.o ddrz.o crctab.o $(LIBS) $(OSLIBS)
	strip ddrz
ddsz:
	$(CC) $(CFLAGS) $(CFLAGS_DDSZ) $(CDEFS) -o ddsz-rbsb.o -c rbsb.c
	$(CC) $(CFLAGS) $(CFLAGS_DDSZ) $(CDEFS) -o ddsz-timing.o -c timing.c
	$(CC) $(CFLAGS) $(CFLAGS_DDSZ) $(CDEFS) -o ddsz-zm.o -c zm.c
	$(CC) $(CFLAGS) $(CFLAGS_DDSZ) $(CDEFS) -o ddsz.o -c ddsz.c	
	$(CC) $(LFLAGS) -o ddsz ddsz-rbsb.o ddsz-timing.o ddsz-zm.o crctab.o ddsz.o $(LIBS) $(OSLIBS)
	strip ddsz

install: 
	cp ddrz ddsz $(INSTALL_PATH)/utils

clean: 
	rm -f *.o ddrz ddsz
