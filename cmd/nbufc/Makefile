CC=gcc
CFLAGS=-I../../include -I../../lib -Wall -g
CFLAGS+=-O2 -flto
LDFLAGS+=-O2 -flto

nbufc: nbufc.o t2b.o lex.yy.o nbuf.tab.o ../../lib/libnbuf.a

nbufc.o: nbufc.c t2b.h ../../include/nbuf.h
lex.yy.o: lex.yy.c lex.yy.h nbuf.tab.h
nbuf.tab.o: nbuf.tab.c nbuf.tab.h ../../include/nbuf.h \
	../../lib/nbuf.nb.h t2b.h

lex.yy.c lex.yy.h: nbuf.l
	flex --header-file=lex.yy.h $<

nbuf.tab.c nbuf.tab.h: nbuf.y
	bison -d $<
