CC=gcc
CFLAGS=-I.. -Wall -g
CFLAGS+=-O2 -flto
LDFLAGS+=-O2 -flto

all: b2t b2h mknbs
b2t: b2t.o load.o
b2h: b2h.o load.o
mknbs: mknbs.o

b2h.o: b2h.c ../nbuf.h nbuf.nb.h
b2t.o: b2t.c ../nbuf.h nbuf.nb.h
load.o: load.c
mknbs.o: mknbs.c ../nbuf.h nbuf.nb.h
