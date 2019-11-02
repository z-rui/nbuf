CC=gcc
CXX=g++
CFLAGS=-g -Wall -O2 -DNDEBUG
CXXFLAGS=$(CFLAGS)
NBUFC=../nbufc/nbufc

all: bench_nb bench_fb bench_pb bench_cp

bench_nb: bench_nb.c dummy.c bench.nb.h
	$(CC) $(CFLAGS) -I../include -o $@ bench_nb.c dummy.c

bench_fb: bench_fb.cc dummy.c bench_fb_generated.h
	$(CXX) $(CXXFLAGS) -o $@ bench_fb.cc dummy.c

bench_pb: bench_pb.cc bench_pb.pb.cc dummy.c bench_pb.pb.h
	$(CXX) $(CXXFLAGS) -o $@ bench_pb.cc bench_pb.pb.cc dummy.c \
		-lprotobuf

bench_cp: bench_cp.cc bench_cp.capnp.c++ bench_cp.capnp.h
	$(CXX) $(CXXFLAGS) -o $@ bench_cp.cc bench_cp.capnp.c++ dummy.c \
		-lcapnp -lkj

%_generated.h: %.fbs
	flatc -c $<

%.nb.h: %.bnbs
	$(NBUFC) h $< > $@

%.bnbs: %.nbs
	$(NBUFC) b $< > $@

%.pb.h %.pb.cc: %.proto
	protoc --cpp_out=. $<

%.capnp.c++ %.capnp.h: %.capnp
	capnp compile -oc++ $<
