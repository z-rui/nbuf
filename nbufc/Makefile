CC=gcc
CFLAGS=-I../include -Wall
CFLAGS+=-g -O2
LDFLAGS+=
AR=ar

nbufc: nbufc.o t2b.o lex.yy.o nbuf.tab.o libnbufc.a
libnbufc.a: b2h.o b2t.o b2c.o b2hh.o
	$(AR) rcs $@ $^

lex.yy.c lex.yy.h: nbuf.l
	flex --header-file=lex.yy.h $<

nbuf.tab.c nbuf.tab.h: nbuf.y
	bison -d $<

b2h.o: b2h.c ../include/nbuf.nb.h ../include/nbuf.h ../include/libnbufc.h
b2hh.o: b2hh.c ../include/nbuf.nb.h ../include/nbuf.h ../include/libnbufc.h
b2c.o: b2c.c ../include/nbuf.nb.h ../include/nbuf.h ../include/libnbufc.h
b2t.o: b2t.c ../include/nbuf.nb.h ../include/nbuf.h ../include/libnbufc.h
lex.yy.o: lex.yy.c nbuf.tab.h
nbuf.tab.o: nbuf.tab.c nbuf.tab.h lex.yy.h t2b.h ../include/nbuf.h
nbufc.o: nbufc.c ../include/nbuf.h ../include/libnbufc.h \
 ../include/nbuf.h t2b.h
t2b.o: t2b.c ../include/nbuf.h ../include/nbuf.nb.h ../include/nbuf.h \
 t2b.h
