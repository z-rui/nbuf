CC=gcc
CFLAGS=-I../include -Wall
CFLAGS+=-g -O2 -fPIC
LDFLAGS+=
AR=ar

nbufc: nbufc.o t2b.o lex.stub.o nbuf.tab.o libnbufc.a ../lib/libnbuf.a

libnbufc.a: b2h.o b2t.o b2c.o b2hh.o
	$(AR) rcs $@ $^

nbuf.tab.c nbuf.tab.h: nbuf.y
	bison -d $<

fileutil.o: fileutil.c
b2c.o: b2c.c ../include/nbuf.nb.h ../include/nbuf.h ../include/nbuf.nb.h \
 ../include/libnbufc.h
b2h.o: b2h.c b2h_common.h ../include/nbuf.nb.h ../include/nbuf.h \
 ../include/nbuf.nb.h ../include/libnbufc.h
b2hh.o: b2hh.c b2h_common.h ../include/nbuf.nb.h ../include/nbuf.h \
 ../include/nbuf.nb.h ../include/libnbufc.h
b2h_main.o: b2h_main.c b2h.c b2h_common.h ../include/nbuf.nb.h \
 ../include/nbuf.h ../include/nbuf.nb.h ../include/libnbufc.h fileutil.c
b2t.o: b2t.c ../include/nbuf.nb.h ../include/nbuf.h ../include/nbuf.nb.h \
 ../include/libnbufc.h
fileutil.o: fileutil.c
lex.stub.o: lex.stub.c nbuf.tab.h ../include/libnbuf.h ../include/nbuf.h \
 ../include/nbuf.nb.h
nbufc.o: nbufc.c ../include/libnbuf.h ../include/nbuf.h \
 ../include/nbuf.nb.h ../include/libnbufc.h t2b.h ../include/nbuf.h \
 nbuf.tab.h
nbuf.tab.o: nbuf.tab.c nbuf.tab.h t2b.h ../include/nbuf.h
t2b.o: t2b.c ../include/nbuf.h ../include/nbuf.nb.h ../include/nbuf.h \
 ../include/nbuf.nb.h t2b.h
