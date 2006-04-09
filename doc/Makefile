DOCS =\
    relation\
    relvar\
    tuple\
    relformat\
    $(NULL)

SRCS = $(patsubst %,ral/%.man,$(DOCS))
HTML = $(patsubst %,HTML/files/%,$(DOCS))
NROFF = $(patsubst %,MAN/files/%.n,$(DOCS))
PS = $(patsubst %,MAN/files/%.ps,$(DOCS))

.PHONY : all html man

all : html man

html : $(SRCS)
	dtplite -o HTML html ral

man : $(SRCS)
	dtplite -o MAN nroff ral
