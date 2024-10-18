include ../makedefs

IFLAGS=-I./ -I../ -I../main -I../lib -I$(PYTHON_INCLUDE)
OPTLFLAGS=-L../lib
CFLAGS=$(WARNFLAGS) $(OPTCFLAGS) $(IFLAGS)
LFLAGS=$(OPTLFLAGS)

LIBS=-ldd

LIBDDPYTHON_OBJ = _dd.o

all: libddpython

libddpython: $(LIBDDPYTHON_OBJ)
	gcc $(LFLAGS) -shared -o ddpmodule.so $(LIBDDPYTHON_OBJ) $(LIBS)
	strip ddpmodule.so

install: 
	cp ddpmodule.so dd.py $(INSTALL_PATH)/python

clean: 
	rm -f *.o ddpmodule.so
