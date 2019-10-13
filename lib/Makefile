CC=gcc
CFLAGS=-I../include -Wall
CFLAGS+=-g -O2 -flto
LDFLAGS+=-g -O2 -flto
AR=gcc-ar

all: libnbuf.a

libnbuf.a: b2h.o b2t.o print.o refl.o
	$(AR) rcs $@ $^

b2h.o: b2h.c ../include/nbuf.h nbuf.nb.h
b2t.o: b2t.c ../include/nbuf.h nbuf.nb.h
print.o: print.c ../include/nbuf.h nbuf.nb.h
refl.o: refl.c ../include/nbuf.h nbuf.nb.h
