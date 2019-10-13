CC=gcc
CFLAGS=-I../include -Wall
CFLAGS+=-g #-O2 -flto
LDFLAGS+=-g #-O2 -flto
AR=gcc-ar

all: libnbuf.a

libnbuf.a: print.o refl.o
	$(AR) rcs $@ $^

print.o: print.c ../include/libnbuf.h ../include/nbuf.h \
 ../include/nbuf.nb.h
refl.o: refl.c ../include/libnbuf.h ../include/nbuf.h \
 ../include/nbuf.nb.h
