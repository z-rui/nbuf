CC=gcc
CFLAGS=-I.. -Wall -g
CFLAGS+=-O2 -flto
LDFLAGS+=-O2 -flto

all: b2t b2h t2b mknbs
b2t: b2t.o load.o
b2h: b2h.o load.o
t2b: t2b.o nbuf.tab.o lex.yy.o
mknbs: mknbs.o

lex.yy.c lex.yy.h: nbuf.l
	flex --header-file=lex.yy.h $<
%.tab.c %.tab.h: %.y
	bison -d $<

b2h.o: b2h.c ../nbuf.h nbuf.nb.h
b2t.o: b2t.c ../nbuf.h nbuf.nb.h
t2b.o: t2b.c ../nbuf.h nbuf.nb.h t2b.h
load.o: load.c
mknbs.o: mknbs.c ../nbuf.h nbuf.nb.h
nbuf.tab.o: nbuf.tab.c nbuf.tab.h lex.yy.h
lex.yy.o: lex.yy.c lex.yy.h nbuf.tab.h t2b.h
