CC=gcc
CFLAGS=-Wall -g
CFLAGS+=-I../include
#CFLAGS+=-fsanitize=address
#LDFLAGS+=-fsanitize=address

LIBNBUF=../lib/libnbuf.a
NBUFC=../nbufc/nbufc

all: nrpc_cli echoserver echo.bnbs

echoserver: echoserver.o nrpc.o
	$(CC) $(LDFLAGS) -o $@ $^ -luv

echoserver.o: echoserver.c echo.nb.h nrpc.h

nrpc_cli: nrpc_cli.o nrpc.o $(LIBNBUF)
	$(CC) $(LDFLAGS) -o $@ $^ -luv

nrpc_cli.o: nrpc_cli.c nrpc.h
nrpc.o: nrpc.c nrpc.h

%.bnbs: %.nbs
	$(NBUFC) b $< > $@

%.nb.h: %.bnbs
	$(NBUFC) h $< > $@
