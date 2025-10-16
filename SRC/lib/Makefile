include ../makedefs

IFLAGS=-I./ -I../ -I../main
CFLAGS=$(WARNFLAGS) $(OPTCFLAGS) $(IFLAGS)

LIBDDCOMMON_OBJ=ddutmpx.o deldir.o setenv.o strlcat.o strlcpy.o unsetenv.o vasprintf.o libddcommon.o md5.o ddmsglib.o
LIBDD_OBJ = libdd.o

all: libddcommon libdd

libddcommon: $(LIBDDCOMMON_OBJ)
	gcc -shared -o libddcommon.so $(LIBDDCOMMON_OBJ)
	ar rcs libddcommon.a $(LIBDDCOMMON_OBJ)
	strip libddcommon.so
libdd: $(LIBDD_OBJ)
	gcc -shared -o libdd.so $(LIBDD_OBJ) libddcommon.a
	ar rcs libdd.a $(LIBDD_OBJ)
	strip libdd.so

install: 
	cp libdd.so libddcommon.so /usr/lib
	ln -s -f /usr/lib/libdd.so /usr/lib/libdd.so.0
	ln -s -f /usr/lib/libddcommon.so /usr/lib/libddcommon.so.0
	cp libdd.so libddcommon.so $(INSTALL_PATH)/lib
	ln -s -f $(INSTALL_PATH)/lib/libdd.so $(INSTALL_PATH)/lib/libdd.so.0
	ln -s -f $(INSTALL_PATH)/lib/libddcommon.so $(INSTALL_PATH)/lib/libddcommon.so.0

clean: 
	rm -f *.o libddcommon.so libdd.so libdd.a libddcommon.a
