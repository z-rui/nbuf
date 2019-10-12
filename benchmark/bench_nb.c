#include "bench.nb.h"

#include <stdio.h>
#include <inttypes.h>

extern void _(const char *, ...);

// #define _ printf

static void print_Foo(Foo *foo)
{
	_("{id = %" PRIx64
	  ", count = %" PRIu16
	  ", prefix = %" PRIu8
	  ", length = %" PRIu32
	  "}",
	  Foo_id(foo), Foo_count(foo), Foo_prefix(foo), Foo_length(foo));
}

static void print_Bar(Bar *bar)
{
	Foo foo = Bar_parent(bar);
	_("{parent = ");
	print_Foo(&foo);
	_(", time = %" PRId32
	  ", ratio = %f"
	  ", size = %" PRIu16
	  "}",
	  Bar_time(bar),
	  Bar_ratio(bar),
	  Bar_size(bar));
}

static void print_FooBar(FooBar *foobar)
{
	Bar bar = FooBar_sibling(foobar);
	_("{sibling = ");
	print_Bar(&bar);
	size_t name_len;
	const char *name = FooBar_name(foobar, &name_len);
	_(", name = %.*s"
	  ", rating = %f"
	  ", postfix = %" PRIu8
	  "}",
	  (int) name_len, name,
	  FooBar_rating(foobar),
	  FooBar_postfix(foobar));
}

static void print_FooBarContainer(FooBarContainer *container)
{
	size_t list_size = FooBarContainer_list_size(container);
	size_t i;
	_("{list = [");
	for (i = 0; i < list_size; i++) {
		FooBar foobar = FooBarContainer_list(container, i);
		print_FooBar(&foobar);
		_(", ");
	}
	size_t location_len;
	const char *location = FooBarContainer_location(container, &location_len);
	_("], initialized = %d"
	  ", fruit = %d"
	  ", location = %.*s"
	  "}",
	  FooBarContainer_initialized(container),
	  (int) FooBarContainer_fruit(container),
	  (int) location_len, location);
}

#include "bench.h"

void test_read(struct nbuf_buffer *buf)
{
	FooBarContainer container = get_FooBarContainer(buf);
	print_FooBarContainer(&container);
	_("\n");
}

void test_write(struct nbuf_buffer *buf)
{
	nbuf_clear(buf);
	const int veclen = 3;
	FooBarContainer container = new_FooBarContainer(buf);
	FooBarContainer_init_list(&container, veclen);
	for (int i = 0; i < veclen; i++) {
		FooBar foobar = FooBarContainer_list(&container, i);
		Bar bar = FooBar_init_sibling(&foobar);
		Foo foo = Bar_init_parent(&bar);
		Foo_set_id(&foo, 0xABADCAFEABADCAFE + i);
		Foo_set_count(&foo, 10000 + i);
		Foo_set_prefix(&foo, '@' + i);
		Foo_set_length(&foo, 1000000 + i);
		Bar_set_time(&bar, 123456 + i);
		Bar_set_ratio(&bar, 3.14159f + i);
		Bar_set_size(&bar, 10000 + i);
		FooBar_set_name(&foobar, "Hello, World!", -1);
		FooBar_set_rating(&foobar, 3.1415432432445543543 + i);
		FooBar_set_postfix(&foobar, '!' + i);
	}
	FooBarContainer_set_initialized(&container, true);
	FooBarContainer_set_fruit(&container, Enum_Bananas);
	FooBarContainer_set_location(&container, "http://github.com/", -1);
}

#include "bench.h"

int main(int argc, char *argv[])
{
	struct nbuf_buffer buf;
	nbuf_init_write(&buf, NULL, 0);
	BENCHMARK(test_write(&buf), 5000000);
	BENCHMARK(test_read(&buf), 5000000);
	FILE *fp = fopen("nb.bin", "w");
	fwrite(buf.base, buf.len, 1, fp);
	fclose(fp);
	nbuf_free(&buf);
}
