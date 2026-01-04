include ../makedefs

# Detect Python versions
PYTHON2 := $(shell which python2 2>/dev/null || which python2.7 2>/dev/null)
PYTHON3 := $(shell which python3 2>/dev/null)
PYTHON2_CONFIG := $(shell which python2-config 2>/dev/null || which python-config 2>/dev/null)
PYTHON3_CONFIG := $(shell which python3-config 2>/dev/null)

# Python 2 settings
ifdef PYTHON2
    ifdef PYTHON2_CONFIG
        PYTHON2_INCLUDE := $(shell $(PYTHON2_CONFIG) --includes)
        PYTHON2_LDFLAGS := $(shell $(PYTHON2_CONFIG) --ldflags)
        PYTHON2_EXT := $(shell $(PYTHON2_CONFIG) --extension-suffix 2>/dev/null || echo ".so")
    else
        # Fallback to PYTHON_INCLUDE from makedefs or use default
        ifneq ($(PYTHON_INCLUDE),)
            PYTHON2_INCLUDE := -I$(PYTHON_INCLUDE)
        else
            PYTHON2_INCLUDE := -I/usr/include/python2.7
        endif
        PYTHON2_LDFLAGS :=
        PYTHON2_EXT := .so
    endif
endif

# Python 3 settings
ifdef PYTHON3
    ifdef PYTHON3_CONFIG
        PYTHON3_INCLUDE := $(shell $(PYTHON3_CONFIG) --includes)
        PYTHON3_LDFLAGS := $(shell $(PYTHON3_CONFIG) --ldflags)
        PYTHON3_EXT := $(shell $(PYTHON3_CONFIG) --extension-suffix 2>/dev/null || echo ".cpython-3.so")
    else
        # Try to detect Python 3 include path
        PYTHON3_INCLUDE := -I/usr/include/python3 -I/usr/local/include/python3
        PYTHON3_LDFLAGS :=
        PYTHON3_EXT := .cpython-3.so
    endif
endif

IFLAGS=-I./ -I../ -I../main -I../lib
OPTLFLAGS=-L../lib
CFLAGS=$(WARNFLAGS) $(OPTCFLAGS) $(IFLAGS)
LFLAGS=$(OPTLFLAGS)

LIBS=-ldd

# Build targets based on available Python interpreters
TARGETS :=
ifdef PYTHON2
    TARGETS += py2
endif
ifdef PYTHON3
    TARGETS += py3
endif

# Default target: build all available
all: $(TARGETS)
	@echo "Build complete!"
	@echo "Python 2: $(if $(PYTHON2),✓ ddpmodule.so,✗ not built)"
	@echo "Python 3: $(if $(PYTHON3),✓ ddp$(PYTHON3_EXT),✗ not built)"

# Python 2 build (keep old name for compatibility)
py2: _dd2.o
	gcc $(LFLAGS) -shared -o ddpmodule.so _dd2.o $(LIBS)
	strip ddpmodule.so

_dd2.o: _dd.c
	$(CC) $(WARNFLAGS) $(OPTCFLAGS) $(IFLAGS) $(PYTHON2_INCLUDE) -c -o $@ $<

# Python 3 build (use ddp name to match import statement)
py3: _dd3.o
	gcc $(LFLAGS) -shared -o ddp$(PYTHON3_EXT) _dd3.o $(LIBS)
	strip ddp$(PYTHON3_EXT)

_dd3.o: _dd.c
	$(CC) $(WARNFLAGS) $(OPTCFLAGS) $(IFLAGS) $(PYTHON3_INCLUDE) -c -o $@ $<

# Legacy target for compatibility
libddpython: all

install: 
	@if [ -f ddpmodule.so ]; then cp ddpmodule.so $(INSTALL_PATH)/python/; fi
	@if [ -f ddp$(PYTHON3_EXT) ]; then cp ddp$(PYTHON3_EXT) $(INSTALL_PATH)/python/; fi
	cp dd.py $(INSTALL_PATH)/python/

clean: 
	rm -f *.o ddpmodule.so ddp*.so

.PHONY: all py2 py3 libddpython install clean
