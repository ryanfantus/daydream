include ../makedefs

IFLAGS=-I./ -I../lib -I../
OPTLFLAGS=-L../lib
CFLAGS=$(WARNFLAGS) $(OPTCFLAGS) $(IFLAGS)
LFLAGS=$(OPTLFLAGS) $(OPTLFLAGS)

LIBS=-lddcommon

DDECHO_OBJ=bslist.o cfg.o date.o fidoaddr.o mb_dd.o mb_msg.o patmat.o pkt.o tosser.o arc.o seenby.o stats.o export.o ftscprod.o dupe.o log.o areafix.o crc.o

all: dd-echo

dd-echo: $(DDECHO_OBJ)
	$(CC) $(LFLAGS) -o ddecho $(DDECHO_OBJ) $(LIBS) $(OSLIBS)
	strip ddecho

install: 
	cp ddecho $(INSTALL_PATH)/utils

clean: 
	rm -f *.o ddecho
