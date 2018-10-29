SUBDIRS	=\
    test\
    x86_64-linux-tcl8.6\
    ix86-linux-tcl8.6\
    x86_64-win64-tcl8.6\
    ix86-win32-tcl8.6\
    $(NULL)

all : packages modules

packages :
	for dir in $(SUBDIRS) ; do\
		$(MAKE) -C $$dir clean;\
		cd $$dir && ./runconfig && cd ..;\
		$(MAKE) -C $$dir;\
		cd $$dir && teapot-pkg generate . && cd ..;\
	done

modules : modules/ral-0.12.2.tm modules/ralutil-0.12.2.tm

modules/ral-0.12.2.tm : ../src/ral.tcl\
	    x86_64-linux-tcl8.6/libral0.12.2.so\
	    x86_64-win64-tcl8.6/ral0122.dll\
	    macosx10.10-i386-x86_64/libral0.12.2.dylib
	@mkdir -p modules
	mkmodule ral 0.12.2\
	    -outdir modules\
	    -script ../src/ral.tcl\
	    -Linux x86_64-linux-tcl8.6/libral0.12.2.so\
	    -Windows x86_64-win64-tcl8.6/ral0122.dll\
	    -Darwin macosx10.10-i386-x86_64/libral0.12.2.dylib

modules/ralutil-0.12.2.tm : ../ralutil/ralutil.tcl
	@mkdir -p modules
	mkmodule ralutil 0.12.2\
	    -outdir modules\
	    -script ../ralutil/ralutil.tcl
