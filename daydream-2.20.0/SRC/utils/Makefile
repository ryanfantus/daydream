include ../makedefs

IFLAGS=-I./ -I../ -I../lib -I../main 
OPTLFLAGS=-L../lib
CFLAGS=$(WARNFLAGS) $(OPTCFLAGS) $(IFLAGS)
LFLAGS=$(OPTLFLAGS)

LIBS=-lddcommon
EXTRA_LIBS=-ldd

DDCALLERS_OBJ=ddcallers.o
DDCFG_OBJ=ddcfg.o
DDFTPWHO_OBJ=ddftpwho.o
DDTOP_OBJ=ddtop.o
DDWHO_OBJ=ddwho.o
DDSNOOP_OBJ=ddsnoop.o
FIXBASES_OBJ=fixbases.o
LISTCLEAN_OBJ=listclean.o
LOGROT_OBJ=logrot.o
MSGBASEOPT_OBJ=msgbaseopt.o
ROBOWRITER_OBJ=robowriter.o
RUNAS_OBJ=runas.o
STRIPANSI_OBJ=stripansi.o
TXTDIZ_OBJ=txtdiz.o
YELLD_OBJ=yelld.o

all: ddcallers ddcfg ddftpwho ddtop ddwho fixbases listclean logrot msgbaseopt robowriter runas stripansi txtdiz yelld ddsnoop pcb2ansi

ddcallers: $(DDCALLERS_OBJ)
	$(CC) $(LFLAGS) -o ddcallers $(DDCALLERS_OBJ) $(LIBS) $(OSLIBS)
	strip ddcallers
ddsnoop: $(DDSNOOP_OBJ)
	$(CC) $(LFLAGS) -o ddsnoop $(DDSNOOP_OBJ) $(LIBS) $(OSLIBS)
	strip ddsnoop
ddcfg: $(DDCFG_OBJ)
	$(CC) $(LFLAGS) -o ddcfg $(DDCFG_OBJ) $(LIBS) $(OSLIBS)
	strip ddcfg
ddftpwho: $(DDFTPWHO_OBJ)
	$(CC) $(LFLAGS) -o ddftpwho $(DDFTPWHO_OBJ) $(LIBS) $(OSLIBS)
	strip ddftpwho
ddtop: $(DDTOP_OBJ)
	$(CC) $(LFLAGS) -o ddtop $(DDTOP_OBJ) $(LIBS) $(OSLIBS)
	strip ddtop
ddwho: $(DDWHO_OBJ)
	$(CC) $(LFLAGS) -o ddwho $(DDWHO_OBJ) $(LIBS) $(OSLIBS)
	strip ddwho
fixbases: $(FIXBASES_OBJ)
	$(CC) $(LFLAGS) -o fixbases $(FIXBASES_OBJ) $(LIBS) $(EXTRA_LIBS) $(OSLIBS) 
	strip fixbases
listclean: $(LISTCLEAN_OBJ)
	$(CC) $(LFLAGS) -o listclean $(LISTCLEAN_OBJ) $(LIBS) $(EXTRA_LIBS) $(OSLIBS)
	strip listclean
msgbaseopt: $(MSGBASEOPT_OBJ)
	$(CC) $(LFLAGS) -o msgbaseopt $(MSGBASEOPT_OBJ) $(LIBS) $(EXTRA_LIBS) $(OSLIBS)
	strip msgbaseopt
robowriter: $(ROBOWRITER_OBJ)
	$(CC) $(LFLAGS) -o robowriter $(ROBOWRITER_OBJ) $(LIBS) $(EXTRA_LIBS) $(OSLIBS)
	strip robowriter
runas: $(RUNAS_OBJ)
	$(CC) $(LFLAGS) -static -o runas $(RUNAS_OBJ) $(LIBS) $(OSLIBS)
	strip runas
stripansi: $(STRIPANSI_OBJ)
	$(CC) $(LFLAGS) -o stripansi $(STRIPANSI_OBJ) $(LIBS) $(OSLIBS)
	strip stripansi
txtdiz: $(TXTDIZ_OBJ)
	$(CC) $(LFLAGS) -o txtdiz $(TXTDIZ_OBJ) $(LIBS) $(OSLIBS)
	strip txtdiz
yelld: $(YELLD_OBJ)
	$(CC) $(LFLAGS) -o yelld $(YELLD_OBJ) $(LIBS) $(OSLIBS)
	strip yelld
logrot: $(LOGROT_OBJ)
	$(CC) $(LFLAGS) -o logrot $(LOGROT_OBJ) $(LIBS) $(OSLIBS)
	strip logrot
install: 
	cp ddcallers ddcfg ddftpwho ddtop ddwho fixbases listclean logrot msgbaseopt robowriter runas stripansi txtdiz yelld ddsnoop $(INSTALL_PATH)/utils

clean: 
	rm -f *.o ddcallers ddcfg ddftpwho ddtop ddwho fixbases listclean logrot msgbaseopt robowriter runas stripansi txtdiz yelld ddsnoop pcb2ansi
