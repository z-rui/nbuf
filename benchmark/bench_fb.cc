#include "bench_fb_generated.h"

#include <cstdio>
#include <cstdlib>
#include <cinttypes>

#include <flatbuffers/flatbuffers.h>

using namespace benchfb;
using flatbuffers::FlatBufferBuilder;
using flatbuffers::Offset;

extern void _(const char *, ...);
extern void load(const char *, void **, size_t *);

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
	print(*foobar.sibling());
	auto name = foobar.name();
	_(", name = %.*s"
	  ", rating = %f"
	  ", postfix = %" PRIu8
	  "}", (int) name->size(), name->c_str(),
	  foobar.rating(), foobar.postfix());
}

void print(const FooBarContainer& container)
{
	_("{list = [");
	for (const FooBar *foobar : *container.list()) {
		print(*foobar);
		_(", ");
	}
	auto location = container.location();
	_("], initialized = %d"
	  ", fruit = %d"
	  ", location = %.*s"
	  "}",
	  container.initialized(),
	  (int) container.fruit(),
	  (int) location->size(), location->c_str());
}

}  // namespace

void test_verify(const FlatBufferBuilder &fbb)
{
	flatbuffers::Verifier v(fbb.GetBufferPointer(), fbb.GetSize());
	bool ok = VerifyFooBarContainerBuffer(v);
	if (!ok)
		abort();
}

void test_read(const FlatBufferBuilder &fbb)
{
	auto container = GetFooBarContainer(fbb.GetBufferPointer());
	print(*container);
	_("\n");
}

// CITE: https://github.com/google/flatbuffers/blob/benchmarks/benchmarks/cpp/FB/benchfb.cpp

void test_write(FlatBufferBuilder &fbb)
{
	fbb.Clear();
	const int veclen = 3;
	Offset<FooBar> vec[veclen];
	for (int i = 0; i < veclen; i++) {
		// We add + i to not make these identical copies for a more realistic
		// compression test.
		auto foo = Foo(0xABADCAFEABADCAFE + i, 10000 + i, '@' + i, 1000000 + i);
		auto bar = Bar(foo, 123456 + i, 3.14159f + i, 10000 + i);
		auto name = fbb.CreateString("Hello, World!");
		auto foobar = CreateFooBar(fbb, &bar, name, 3.1415432432445543543 + i,
				'!' + i);
		vec[i] = foobar;
	}
	auto location = fbb.CreateString("http://github.com/");
	auto foobarvec = fbb.CreateVector(vec, veclen);
	auto foobarcontainer = CreateFooBarContainer(fbb, foobarvec, true,
			Enum_Bananas, location);
	fbb.Finish(foobarcontainer);
}

#include "bench.h"

int main(int argc, char *argv[])
{
	FlatBufferBuilder buf;
	BENCHMARK(test_write(buf), 5000000);
	BENCHMARK(test_verify(buf), 5000000);
	BENCHMARK(test_read(buf), 5000000);
	FILE *fp = fopen("fb.bin", "w");
	fwrite(buf.GetBufferPointer(), buf.GetSize(), 1, fp);
	fclose(fp);
	return 0;
}
