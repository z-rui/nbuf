#include "bench_pb.pb.h"

#include <cstdio>
#include <cinttypes>
#include <fstream>
#include <string>
#include <memory>

#include <google/protobuf/arena.h>
#include <google/protobuf/io/zero_copy_stream_impl.h>

using namespace benchpb;
using google::protobuf::Arena;

extern void _(const char *, ...);

// #define _ printf

namespace {

void print(const Foo& foo)
{
	_("{id = %" PRIx64
	  ", count = %" PRIu16
	  ", prefix = %" PRIu8
	  ", length = %" PRIu32
	  "}", foo.id(), foo.count(), foo.prefix(), foo.length());
}

void print(const Bar& bar)
{
	_("{parent = ");
	print(bar.parent());
	_(", time = %" PRId32
	  ", ratio = %f"
	  ", size = %" PRIu16
	  "}", bar.time(), bar.ratio(), bar.size());
}

void print(const FooBar& foobar)
{
	_("{sibling = ");
	print(foobar.sibling());
	auto name = foobar.name();
	_(", name = %.*s"
	  ", rating = %f"
	  ", postfix = %" PRIu8
	  "}", (int) name.size(), name.c_str(),
	  foobar.rating(), foobar.postfix());
}

void print(const FooBarContainer& container)
{
	_("{list = [");
	for (FooBar foobar : container.list()) {
		print(foobar);
		_(", ");
	}
	auto location = container.location();
	_("], initialized = %d"
	  ", fruit = %d"
	  ", location = %.*s"
	  "}",
	  container.initialized(),
	  (int) container.fruit(),
	  (int) location.size(), location.c_str());
}

}  // namespace

static FooBarContainer *fbc;

void test_use()
{
	print(*fbc);
	_("\n");
}

void test_create(Arena *arena)
{
	arena->Reset();
	const int veclen = 3;
	fbc = Arena::CreateMessage<FooBarContainer>(arena);
	auto list = fbc->mutable_list();
	for (int i = 0; i < veclen; i++) {
		auto foobar = list->Add();
		auto foo = foobar->mutable_sibling()->mutable_parent();
		foo->set_id(0xABADCAFEABADCAFE + i);
		foo->set_count(10000 + i);
		foo->set_prefix('@' + i);
		foo->set_length(1000000 + i); 
		auto bar = foobar->mutable_sibling(); // this shouldn't invalidate bar->parent()

		bar->set_time(123456 + i);
		bar->set_ratio(3.14159f + i);
		bar->set_size(10000 + i);
		foobar->set_name("Hello, World!");
		foobar->set_rating(3.1415432432445543543 + i);
		foobar->set_postfix('!' + i);
	}
	fbc->set_initialized(true);
	fbc->set_fruit(Enum::Bananas);
	fbc->set_location("http://github.com/");
}

void test_serialize(std::string *buf)
{
	fbc->SerializeToString(buf);
}

void test_deserialize(const std::string buf)
{
	fbc->ParseFromString(buf);
}

#include "bench.h"

int main(int argc, char *argv[])
{
	std::string buf;
	Arena arena;
	BENCHMARK(test_create(&arena), 1000000);
	BENCHMARK(test_serialize(&buf), 1000000);
	BENCHMARK(test_deserialize(buf), 1000000);
	BENCHMARK(test_use(), 1000000);
	std::fstream f("pb.bin", std::ios::out | std::ios::binary);
	google::protobuf::io::OstreamOutputStream s(&f);
	fbc->SerializeToZeroCopyStream(&s);
}
