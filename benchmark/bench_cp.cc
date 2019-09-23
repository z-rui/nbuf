#include "bench_cp.capnp.h"

#include <cstdio>
#include <cstdlib>
#include <cinttypes>
#include <memory>
#include <utility>

#include <capnp/serialize.h>

void _(const char *, ...);

// #define _ printf

namespace {

void print(const Foo::Reader& foo)
{
	_("{id = %" PRIx64
	  ", count = %" PRIu16
	  ", prefix = %" PRIu8
	  ", length = %" PRIu32
	  "}", foo.getId(), foo.getCount(), foo.getPrefix(), foo.getLength());
}

void print(const Bar::Reader& bar)
{
	_("{parent = ");
	print(bar.getParent());
	_(", time = %" PRId32
	  ", ratio = %f"
	  ", size = %" PRIu16
	  "}", bar.getTime(), bar.getRatio(), bar.getSize());
}

void print(const FooBar::Reader& foobar)
{
	_("{sibling = ");
	print(foobar.getSibling());
	auto name = foobar.getName();
	_(", name = %.*s"
	  ", rating = %f"
	  ", postfix = %" PRIu8
	  "}", (int) name.size(), name.cStr(),
	  foobar.getRating(), foobar.getPostfix());
}

void print(const FooBarContainer::Reader& container)
{
	_("{list = [");
	for (auto foobar : container.getList()) {
		print(foobar);
		_(", ");
	}
	auto location = container.getLocation();
	_("], initialized = %d"
	  ", fruit = %d"
	  ", location = %.*s"
	  "}",
	  container.getInitialized(),
	  (int) container.getFruit(),
	  (int) location.size(), location.cStr());
}

}

void test_read(capnp::MessageBuilder &buf)
{
	auto container = buf.getRoot<FooBarContainer>();
	print(container);
	_("\n");
}

void test_write(std::unique_ptr<capnp::MessageBuilder> &buf)
{
	buf = std::make_unique<capnp::MallocMessageBuilder>();
	auto container = buf->initRoot<FooBarContainer>();
	const int veclen = 3;
	auto list = container.initList(veclen);
	for (int i = 0; i < veclen; i++) {
		auto foobar = list[i];
		auto foo = foobar.getSibling().getParent();
		foo.setId(0xABADCAFEABADCAFE + i);
		foo.setCount(10000 + i);
		foo.setPrefix('@' + i);
		foo.setLength(1000000 + i); 
		auto bar = foobar.getSibling(); // this shouldn't invalidate bar.parent()

		bar.setTime(123456 + i);
		bar.setRatio(3.14159f + i);
		bar.setSize(10000 + i);
		foobar.setName("Hello, World!");
		foobar.setRating(3.1415432432445543543 + i);
		foobar.setPostfix('!' + i);
	}
	container.setInitialized(true);
	container.setFruit(Enum::BANANAS);
	container.setLocation("http://github.com/");
}

#include "bench.h"

int main()
{
	std::unique_ptr<capnp::MessageBuilder> buf;
	BENCHMARK(test_write(buf), 2000000);
	BENCHMARK(test_read(*buf), 2000000);
	FILE *fp = fopen("cp.bin", "w");
	writeMessageToFd(fileno(fp), *buf);
	fclose(fp);
}
