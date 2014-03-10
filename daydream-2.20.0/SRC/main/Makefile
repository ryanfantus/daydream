include ../makedefs

IFLAGS=-I./ -I../ -I../lib
OPTLFLAGS=-L../lib
CFLAGS=$(WARNFLAGS) $(OPTCFLAGS) $(IFLAGS)
LFLAGS=$(OPTLFLAGS)

LIBS=-lddcommon

DAYDREAM_OBJ=access.o bulletin.o conference.o console.o daydream.o default.o display.o domenu.o\
doorport.o download.o edituser.o enterbbs.o entermsg.o filelist.o goodbye.o hotkey.o\
joinconf.o linechat.o lineed.o lists.o mailscan.o menus.o misc.o movefile.o newuser.o\
olm.o page.o prompt.o readmsgs.o replymsg.o stats.o stdiohan.o symtab.o tagedit.o\
textsearch.o typetext.o unix.o upload.o user.o usered.o userlist.o utility.o version.o\
viewfile.o wfc.o who.o ftpul.o

all: daydream

daydream: $(DAYDREAM_OBJ)
	$(CC) $(LFLAGS) -o daydream $(DAYDREAM_OBJ) $(LIBS) $(OSLIBS)
	strip daydream

install: 
	cp daydream $(INSTALL_PATH)/bin

clean: 
	rm -f *.o daydream
