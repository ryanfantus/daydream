include ../makedefs

IFLAGS=-I./ -I../ -I../lib -I../main
OPTLFLAGS=-L../lib
CXXFLAGS=$(WARNFLAGS) $(OPTCFLAGS) $(IFLAGS)
LXXFLAGS=$(OPTLFLAGS)

LIBS=-lddcommon

DDTICK_OBJ=main.o announce.o tic.o funcs.o patmat.o cfg.o toss.o request.o fidoaddr.o log.o file.o outbound.o directory.o

all: ddtick

ddtick: $(DDTICK_OBJ)
	$(CXX) $(LXXFLAGS) -o ddtick $(DDTICK_OBJ) $(LIBS) $(OSLIBS)
	strip ddtick

install: 
	cp ddtick $(INSTALL_PATH)/utils/

clean: 
	rm -f *.o ddtick
