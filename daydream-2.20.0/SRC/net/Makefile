include ../makedefs

IFLAGS=-I./ -I../ -I../lib -I../main 
OPTLFLAGS=-L../lib
CFLAGS=$(WARNFLAGS) $(OPTCFLAGS) $(IFLAGS)
LFLAGS=$(OPTLFLAGS)
YACC=./fakeyacc

LIBS=-lddcommon
EXTRA_LIBS=-ldd

DDFTPD_OBJ=ddftpd.o popen.o ftpcmd.o md5.o
DDTELNETD_OBJ=ddtelnetd.o

all: ddftpd ddtelnetd 

ddftpd: $(DDFTPD_OBJ)
	$(CC) $(LFLAGS) -o ddftpd $(DDFTPD_OBJ) $(LIBS)
	strip ddftpd
ddtelnetd: $(DDTELNETD_OBJ)
	$(CC) $(LFLAGS) -o ddtelnetd $(DDTELNETD_OBJ) -lutil
	strip ddtelnetd
install: 
	cp ddtelnetd ddftpd $(INSTALL_PATH)/bin

clean: 
	rm -f *.o ddtelnetd ddftpd
