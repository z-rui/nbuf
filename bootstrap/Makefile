CC=gcc
CFLAGS=-I../include -Wall -g
CFLAGS+=-O2 -flto
LDFLAGS+=-O2 -flto

all: mknbs
mknbs: mknbs.o

mknbs.o: mknbs.c ../include/nbuf.h
