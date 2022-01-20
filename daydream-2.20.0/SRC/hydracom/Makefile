include ../makedefs

CFLAGS_HYDRA=-DHYDRA -DHAVE_CONFIG_H -D_GNU_SOURCE
IFLAGS=-I./ -I../ -I../lib -I../main
OPTLFLAGS=-L../lib
LIBS=-lddcommon -lcurses
CFLAGS=$(WARNFLAGS) $(OPTCFLAGS) $(IFLAGS)
LFLAGS=$(OPTLFLAGS)

all: hydracom

hydracom:
	$(CC) $(CFLAGS) $(CFLAGS_HYDRA) $(CDEFS) -o dos_file.o -c dos_file.c
	$(CC) $(CFLAGS) $(CFLAGS_HYDRA) $(CDEFS) -o fmisc.o -c fmisc.c
	$(CC) $(CFLAGS) $(CFLAGS_HYDRA) $(CDEFS) -o hydra.o -c hydra.c
	$(CC) $(CFLAGS) $(CFLAGS_HYDRA) $(CDEFS) -o hydracom.o -c hydracom.c
	$(CC) $(CFLAGS) $(CFLAGS_HYDRA) $(CDEFS) -o misc.o -c misc.c
	$(CC) $(CFLAGS) $(CFLAGS_HYDRA) $(CDEFS) -o unix.o -c unix.c
	$(CC) $(LFLAGS) -o hydracom dos_file.o fmisc.o hydra.o hydracom.o misc.o unix.o $(LIBS) $(OSLIBS)
	strip hydracom

install:
	cp hydracom $(INSTALL_PATH)/utils

clean:
	rm -rf *.o hydracom
