CFLAGS = -g -I/usr/local/ActiveTcl/include
LDFLAGS = -shared -g
LOADLIBES = -L/usr/local/ActiveTcl/lib
LDLIBS = -ltclstub8.5

libral.so : ral.o
	$(CC) -o $@ $(LDFLAGS) $^ $(LOADLIBES) $(LDLIBS)

rla.o : /usr/local/ActiveTcl/include/tcl.h
