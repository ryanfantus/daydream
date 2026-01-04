include ../makedefs

# Detect Python versions
PYTHON2_CONFIG := $(shell which python2-config 2>/dev/null || which python-config 2>/dev/null)
PYTHON3_CONFIG := $(shell which python3-config 2>/dev/null)

# Python 2 settings (fallback to PYTHON_INCLUDE from makedefs)
ifdef PYTHON2_CONFIG
    PYTHON2_INCLUDE := $(shell $(PYTHON2_CONFIG) --includes)
    PYTHON2_LDFLAGS := $(shell $(PYTHON2_CONFIG) --ldflags)
    PYTHON2_EXT := $(shell $(PYTHON2_CONFIG) --extension-suffix 2>/dev/null || echo ".so")
else
    PYTHON2_INCLUDE := -I$(PYTHON_INCLUDE)
    PYTHON2_LDFLAGS :=
    PYTHON2_EXT := .so
endif

# Python 3 settings
ifdef PYTHON3_CONFIG
    PYTHON3_INCLUDE := $(shell $(PYTHON3_CONFIG) --includes)
    PYTHON3_LDFLAGS := $(shell $(PYTHON3_CONFIG) --ldflags)
    PYTHON3_EXT := $(shell $(PYTHON3_CONFIG) --extension-suffix 2>/dev/null || echo ".cpython-3.so")
endif

IFLAGS=-I./ -I../ -I../main -I../lib
OPTLFLAGS=-L../lib
CFLAGS=$(WARNFLAGS) $(OPTCFLAGS) $(IFLAGS)
LFLAGS=$(OPTLFLAGS)

LIBS=-ldd

# Build targets based on available Python versions
TARGETS :=
ifdef PYTHON2_CONFIG
    TARGETS += py2
endif
ifdef PYTHON3_CONFIG
    TARGETS += py3
endif

# Default target: build all available
all: $(TARGETS)

# Python 2 build
py2: _dd2.o
	gcc $(LFLAGS) -shared -o ddpmodule$(PYTHON2_EXT) _dd2.o $(LIBS)
	strip ddpmodule$(PYTHON2_EXT)

_dd2.o: _dd.c
	$(CC) $(WARNFLAGS) $(OPTCFLAGS) $(IFLAGS) $(PYTHON2_INCLUDE) -c -o $@ $<

# Python 3 build
py3: _dd3.o
	gcc $(LFLAGS) -shared -o ddpmodule$(PYTHON3_EXT) _dd3.o $(LIBS)
	strip ddpmodule$(PYTHON3_EXT)

_dd3.o: _dd.c
	$(CC) $(WARNFLAGS) $(OPTCFLAGS) $(IFLAGS) $(PYTHON3_INCLUDE) -c -o $@ $<

# Legacy target for compatibility
libddpython: all

install: 
	@if [ -f ddpmodule.so ]; then cp ddpmodule.so $(INSTALL_PATH)/python/; fi
	@if [ -f ddpmodule$(PYTHON3_EXT) ]; then cp ddpmodule$(PYTHON3_EXT) $(INSTALL_PATH)/python/; fi
	cp dd.py $(INSTALL_PATH)/python/

clean: 
	rm -f *.o ddpmodule.so ddpmodule*.so

.PHONY: all py2 py3 libddpython install clean
