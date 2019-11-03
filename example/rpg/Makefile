CC=gcc
NBUF_DIR=../..
CFLAGS=-I$(NBUF_DIR)/include
CFLAGS+=-Wall -g
NBUFC=$(NBUF_DIR)/nbufc/nbufc

all: config.bin main

main: main.o common.o $(NBUF_DIR)/lib/libnbuf.a

%.bnbs: %.nbs
	$(NBUFC) b $< > $@

%.nb.h: %.bnbs
	$(NBUFC) h $< > $@

config.bin: config.textnb rpg.bnbs
	$(NBUFC) e rpg.bnbs GameConfig < $< > $@

common.o: common.c common.h rpg.nb.h ../../include/nbuf.h \
 ../../include/nbuf.nb.h ../../include/nbuf.h ../../include/nbuf.nb.h \
 ../../include/libnbuf.h
entry.o: entry.c common.h rpg.nb.h ../../include/nbuf.h \
 ../../include/nbuf.nb.h ../../include/nbuf.h ../../include/nbuf.nb.h
main.o: main.c rpg.nb.h ../../include/nbuf.h ../../include/nbuf.nb.h \
 ../../include/nbuf.h ../../include/nbuf.nb.h common.h
