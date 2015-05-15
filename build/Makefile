SUBDIRS	=\
    test\
    x86_64-linux-tcl8.6\
    ix86-linux-tcl8.6\
    x86_64-win64-tcl8.6\
    ix86-win32-tcl8.6\
    $(NULL)

all :
	for dir in $(SUBDIRS) ; do\
		$(MAKE) -C $$dir clean;\
		cd $$dir && ./runconfig && cd ..;\
		$(MAKE) -C $$dir;\
		cd $$dir && teapot-pkg generate . && cd ..;\
	done
