CC=gcc
CXX=g++
CFLAGS=-g -Wall -O2 -DNDEBUG
CXXFLAGS=$(CFLAGS)

all: bench_nb bench_fb bench_pb bench_cp

# bench_nb: bench_nb.cc dummy.c bench_nb_generated.hpp
# 	$(CXX) $(CXXFLAGS) -I.. -o $@ bench_nb.cc dummy.c

bench_nb: bench_nb.c dummy.c bench_nb_generated.h
	$(CC) $(CFLAGS) -I.. -o $@ bench_nb.c dummy.c

bench_fb: bench_fb.cc dummy.c bench_fb_generated.h
	$(CXX) $(CXXFLAGS) -o $@ bench_fb.cc dummy.c

bench_pb: bench_pb.cc bench_pb.pb.cc dummy.c bench_pb.pb.h
	$(CXX) $(CXXFLAGS) -o $@ bench_pb.cc bench_pb.pb.cc dummy.c \
		-lprotobuf

bench_cp: bench_cp.cc bench_cp.capnp.c++ bench_cp.capnp.h
	$(CXX) $(CXXFLAGS) -o $@ bench_cp.cc bench_cp.capnp.c++ dummy.c \
		-lcapnp -lkj

bench_fb_generated.h: bench_fb.fbs
	flatc -c bench_fb.fbs

bench_pb.pb.h bench_pb.pb.cc: bench_pb.proto
	protoc --cpp_out=. bench_pb.proto

bench_cp.capnp.c++ bench_cp.capnp.h: bench_cp.capnp
	capnp compile -oc++ bench_cp.capnp
