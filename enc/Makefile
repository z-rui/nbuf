CC=gcc
CFLAGS=-Wall -I../include
CFLAGS+=-g -O2
#CFLAGS+=-fsanitize=address
#LDFLAGS+=-fsanitize=address

all: enc dec

enc: enc.o ../lib/libnbuf.a
	$(CC) $(LDFLAGS) -o $@ $^

dec: dec.o ../lib/libnbuf.a
	$(CC) $(LDFLAGS) -o $@ $^

%.tab.c %.tab.h: %.y
	bison -dv $<

dec.o: dec.c ../include/libnbuf.h ../include/nbuf.h ../include/nbuf.nb.h
enc.o: enc.c ../include/libnbuf.h ../include/nbuf.h ../include/nbuf.nb.h
