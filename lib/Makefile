CC=gcc
CFLAGS=-I../include -Wall
CFLAGS+=-g -O2
LDFLAGS+=
AR=ar

all: libnbuf.a

libnbuf.a: print.o refl.o pool.o util.o token.o parse.o
	$(AR) rcs $@ $^
parse.o: parse.c ../include/libnbuf.h ../include/nbuf.h \
 ../include/nbuf.nb.h
pool.o: pool.c ../include/libnbuf.h ../include/nbuf.h \
 ../include/nbuf.nb.h
print.o: print.c ../include/libnbuf.h ../include/nbuf.h \
 ../include/nbuf.nb.h
refl.o: refl.c ../include/libnbuf.h ../include/nbuf.h \
 ../include/nbuf.nb.h
token.o: token.c ../include/libnbuf.h ../include/nbuf.h \
 ../include/nbuf.nb.h
util.o: util.c ../include/nbuf.h
