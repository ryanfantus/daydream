include ../makedefs

IFLAGS=-I./ -I../ -I../lib -I../main
OPTLFLAGS=-L../lib
LIBS=-ldd 
CFLAGS=$(WARNFLAGS) $(OPTCFLAGS) $(IFLAGS)
LFLAGS=$(OPTLFLAGS) $(OPTLFLAGS)

AUTOSIG_OBJ=autosig.o
CONFTOP_OBJ=conftop.o
DDED_OBJ=dded.o
FAKER_OBJ=faker.o
GRAB_OBJ=grab.o
LOG_OBJ=log.o
NEW_OBJ=new.o
SETLRP_OBJ=setlrp.o
WALL_OBJ=wall.o
WHOF_OBJ=whof.o

all: autosig conftop dded faker grab log new setlrp wall whof


autosig: $(AUTOSIG_OBJ)
	$(CC) $(LFLAGS) -o autosig $(AUTOSIG_OBJ) $(LIBS) $(OSLIBS)
	strip autosig
conftop: $(CONFTOP_OBJ)
	$(CC) $(LFLAGS) -o conftop $(CONFTOP_OBJ) $(LIBS) $(OSLIBS)
	strip conftop
dded: $(DDED_OBJ)
	$(CC) $(LFLAGS) -o dded $(DDED_OBJ) $(LIBS) $(OSLIBS)
	strip dded
faker: $(FAKER_OBJ)
	$(CC) $(LFLAGS) -o faker $(FAKER_OBJ) $(LIBS) $(OSLIBS)
	strip faker
grab: $(GRAB_OBJ)
	$(CC) $(LFLAGS) -o grab $(GRAB_OBJ) $(LIBS) $(OSLIBS)
	strip grab
log: $(LOG_OBJ)
	$(CC) $(LFLAGS) -o log $(LOG_OBJ) $(LIBS) $(OSLIBS)
	strip log
new: $(NEW_OBJ)
	$(CC) $(LFLAGS) -o new $(NEW_OBJ) $(LIBS) $(OSLIBS)
	strip new
setlrp: $(SETLRP_OBJ)
	$(CC) $(LFLAGS) -o setlrp $(SETLRP_OBJ) $(LIBS) $(OSLIBS)
	strip setlrp
wall: $(WALL_OBJ)
	$(CC) $(LFLAGS) -o wall $(WALL_OBJ) $(LIBS) $(OSLIBS)
	strip wall
whof: $(WHOF_OBJ)
	$(CC) $(LFLAGS) -o whof $(WHOF_OBJ) $(LIBS) $(OSLIBS)
	strip whof

install: 
	cp autosig conftop dded faker grab log new setlrp wall whof $(INSTALL_PATH)/doors

clean: 
	rm -f *.o autosig conftop dded faker grab log new setlrp wall whof
