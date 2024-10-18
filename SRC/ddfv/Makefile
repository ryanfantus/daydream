include ../makedefs

IFLAGS=-I./ -I../ -I../lib -I../main
OPTLFLAGS=-L../lib
CXXFLAGS=$(WARNFLAGS) $(OPTCFLAGS) $(IFLAGS)
LXXFLAGS=$(OPTLFLAGS)

LIBS=-ldd

DDFV_OBJ=parser.o listviewer.o listentry.o lightbar.o libddc++.o filelist.o extract.o entrypack.o\
dirlist.o direntry.o ddfv.o common.o

all: ddfv

ddfv: $(DDFV_OBJ)
	$(CXX) $(LXXFLAGS) -o ddfv $(DDFV_OBJ) $(LIBS) $(OSLIBS)
	strip ddfv

install: 
	cp ddfv $(INSTALL_PATH)/doors

clean: 
	rm -f *.o ddfv
