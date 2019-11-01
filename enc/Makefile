CC=gcc
CFLAGS=-Wall -I../include
CFLAGS+=-g -O2
#CFLAGS+=-DYYDEBUG
#CFLAGS+=-fsanitize=address
#LDFLAGS+=-fsanitize=address

all: enc dec

enc: textnb.tab.o lex.yy.o enc.o ../lib/libnbuf.a
	$(CC) $(LDFLAGS) -o $@ $^

dec: dec.o ../lib/libnbuf.a
	$(CC) $(LDFLAGS) -o $@ $^

%.tab.c %.tab.h: %.y
	bison -dv $<

dec.o: dec.c ../include/libnbuf.h ../include/nbuf.h ../include/nbuf.nb.h
enc.o: enc.c textnb.tab.h ../include/nbuf.h ../include/libnbuf.h \
 ../include/nbuf.h ../include/nbuf.nb.h
lex.yy.o: lex.yy.c textnb.tab.h ../include/nbuf.h ../include/libnbuf.h \
 ../include/nbuf.h ../include/nbuf.nb.h
textnb.tab.o: textnb.tab.c textnb.tab.h ../include/nbuf.h \
 ../include/libnbuf.h ../include/nbuf.h ../include/nbuf.nb.h
