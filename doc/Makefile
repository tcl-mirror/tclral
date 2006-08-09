DOCS =\
    relvar\
    relation\
    tuple\
    relformat\
    serialize\
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
	dtplite -o MAN -ext n nroff ral
