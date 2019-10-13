CC=gcc
CFLAGS=-I../../include -Wall
CFLAGS+=-g -O2 -flto
LDFLAGS+=-g -O2 -flto

nbufc: nbufc.o t2b.o lex.yy.o nbuf.tab.o ../../lib/libnbuf.a

lex.yy.c lex.yy.h: nbuf.l
	flex --header-file=lex.yy.h $<

nbuf.tab.c nbuf.tab.h: nbuf.y
	bison -d $<

lex.yy.o: lex.yy.c nbuf.tab.h
nbuf.tab.o: nbuf.tab.c nbuf.tab.h lex.yy.h t2b.h ../../include/nbuf.h
nbufc.o: nbufc.c ../../include/nbuf.h ../../include/libnbuf.h \
 ../../include/nbuf.h ../../include/nbuf.nb.h t2b.h
t2b.o: t2b.c ../../include/nbuf.h ../../include/nbuf.nb.h \
 ../../include/nbuf.h t2b.h
