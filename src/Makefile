TCLBASE = /usr/local/ActiveTcl

CFLAGS = -g -I$(TCLBASE)/include
LDFLAGS = -shared -g
LOADLIBES = -L$(TCLBASE)/lib
LDLIBS = -ltclstub8.5

libral.so : ral.o
	$(CC) -o $@ $(LDFLAGS) $^ $(LOADLIBES) $(LDLIBS)

rla.o : $(TCLBASE)/include/tcl.h
