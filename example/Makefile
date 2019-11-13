CC=gcc
CXX=g++
CFLAGS=-Wall -I../include
CFLAGS+=-g -O2
CXXFLAGS=$(CFLAGS)
LDFLAGS+=
NBUFC=../nbufc/nbufc
B2PY=../python/b2py.py

all: printer writer my_game_nb.py my_game_pb

printer: printer.o my_game.nb.o ../lib/libnbuf.a
	$(CC) $(LDFLAGS) -o $@ $^

writer: writer.o my_game.nb.o ../lib/libnbuf.a
	$(CC) $(LDFLAGS) -o $@ $^

printer.o: printer.c my_game.nb.h ../include/nbuf.h
writer.o: writer.cc my_game.nb.hpp ../include/nbuf.h

%.nb.c: %.bnbs
	$(NBUFC) c $< > $@
%.nb.h: %.bnbs
	$(NBUFC) h $< > $@
%.nb.hpp: %.bnbs
	$(NBUFC) H $< > $@
%_nb.py: %.bnbs
	$(B2PY) $< > $@
	
%.bnbs: %.nbs
	$(NBUFC) b $< > $@

my_game_pb: my_game_pb.o my_game.pb.o
	$(CXX) $(LDFLAGS) -o $@ $^ -lprotobuf -lpthread

my_game_pb.o: my_game_pb.cc my_game.pb.h

my_game.pb.o: my_game.pb.cc my_game.pb.h

my_game.pb.cc my_game.pb.h: my_game.proto
	protoc --cpp_out=. $<
