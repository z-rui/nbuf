CC=gcc
CXX=g++
CFLAGS=-Wall -I../include
CFLAGS+=-g -O2
CXXFLAGS=$(CFLAGS)
LDFLAGS+=
NBUFC=../nbufc/nbufc

all: my_game my_game++ my_game_pb
my_game: my_game.o my_game.nb.o ../lib/libnbuf.a
	$(CC) $(LDFLAGS) -o $@ $^

my_game++: my_game++.o my_game.nb.o ../lib/libnbuf.a
	$(CC) $(LDFLAGS) -o $@ $^

my_game.o: my_game.c my_game.nb.h ../include/nbuf.h
my_game++.o: my_game++.cc my_game.nb.hpp ../include/nbuf.h

%.nb.c: %.bnbs
	$(NBUFC) c $< > $@
%.nb.h: %.bnbs
	$(NBUFC) h $< > $@
%.nb.hpp: %.bnbs
	$(NBUFC) H $< > $@
%.bnbs: %.nbs
	$(NBUFC) b $< > $@

my_game_pb: my_game_pb.o my_game.pb.o
	$(CXX) $(LDFLAGS) -o $@ $^ -lprotobuf -lpthread

my_game_pb.o: my_game_pb.cc my_game.pb.h

my_game.pb.o: my_game.pb.cc my_game.pb.h

my_game.pb.cc my_game.pb.h: my_game.proto
	protoc --cpp_out=. $<
