CC=gcc
CFLAGS=-I../include -Wall -g
CFLAGS+=-O2 -flto
LDFLAGS+=-O2 -flto
AR=gcc-ar

all: libnbuf.a

libnbuf.a: b2h.o b2t.o
	$(AR) rcs $@ $^

b2h.o: b2h.c ../include/nbuf.h nbuf.nb.h
b2t.o: b2t.c ../include/nbuf.h nbuf.nb.h
