DOCS =\
    relvar\
    relation\
    tuple\
    relformat\
    serialize\
    ralutil\
    $(NULL)

SRCS = $(patsubst %,ral/%.man,$(DOCS))
HTML = $(patsubst %,HTML/files/%,$(DOCS))
NROFF = $(patsubst %,MAN/files/%.n,$(DOCS))

.PHONY : all html man

all : html man

html : $(SRCS)
	dtplite -o HTML html ral

man : $(SRCS)
	dtplite -o MAN -ext n nroff ral

zip : 
	zip -r ral-0.11.4-html.zip HTML
