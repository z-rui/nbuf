CC=gcc
CFLAGS=-I../include -Wall -g
CFLAGS+=-O2 -flto
LDFLAGS+=-O2 -flto

bootstrap: bootstrap.o

bootstrap.o: bootstrap.c bootstrap.h ../include/nbuf.h
