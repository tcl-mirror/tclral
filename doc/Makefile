SRCS =\
    doctools_fmt\
    relation\
    relvar\
    tuple\
    relformat\
    $(NULL)

HTML = $(addsuffix .html,$(SRCS))
NROFF = $(addsuffix .nroff,$(SRCS))
PS = $(addsuffix .ps,$(SRCS))

all : $(HTML) $(PS)

%.html : %.man ; ./mpexpand html $< $@
%.nroff : %.man ; ./mpexpand nroff $< $@
%.ps : %.nroff ; groff -man $< > $@
