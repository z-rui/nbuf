/* Generated from bench.nb.  DO NOT EDIT. */

#ifndef BENCH_NB_H
#define BENCH_NB_H

#include "nbuf.h"

typedef enum {
	Enum_Apples = 0,
	Enum_Pears = 1,
	Enum_Bananas = 2,
} Enum;

typedef struct {
	struct nbuf_obj o;
} Foo;

static inline Foo
get_Foo(struct nbuf_buffer *buf)
{
	Foo o = {{buf}};
	nbuf_obj_init(&o.o, 0);
	return o;
}

static inline Foo
new_Foo(struct nbuf_buffer *buf)
{
	Foo o;
	o.o = nbuf_create(buf, 1, 16, 0);
	return o;
}

typedef struct {
	struct nbuf_obj o;
} Bar;

static inline Bar
get_Bar(struct nbuf_buffer *buf)
{
	Bar o = {{buf}};
	nbuf_obj_init(&o.o, 0);
	return o;
}

static inline Bar
new_Bar(struct nbuf_buffer *buf)
{
	Bar o;
	o.o = nbuf_create(buf, 1, 12, 1);
	return o;
}

typedef struct {
	struct nbuf_obj o;
} FooBar;

static inline FooBar
get_FooBar(struct nbuf_buffer *buf)
{
	FooBar o = {{buf}};
	nbuf_obj_init(&o.o, 0);
	return o;
}

static inline FooBar
new_FooBar(struct nbuf_buffer *buf)
{
	FooBar o;
	o.o = nbuf_create(buf, 1, 12, 2);
	return o;
}

typedef struct {
	struct nbuf_obj o;
} FooBarContainer;

static inline FooBarContainer
get_FooBarContainer(struct nbuf_buffer *buf)
{
	FooBarContainer o = {{buf}};
	nbuf_obj_init(&o.o, 0);
	return o;
}

static inline FooBarContainer
new_FooBarContainer(struct nbuf_buffer *buf)
{
	FooBarContainer o;
	o.o = nbuf_create(buf, 1, 4, 2);
	return o;
}

static inline uint64_t
Foo_id(const Foo *o)
{
	return nbuf_get_int(&o->o, 0, 8);
}

static inline void
Foo_set_id(const Foo *o, uint64_t v)
{
	return nbuf_put_int(&o->o, 0, 8, v);
}

static inline uint16_t
Foo_count(const Foo *o)
{
	return nbuf_get_int(&o->o, 8, 2);
}

static inline void
Foo_set_count(const Foo *o, uint16_t v)
{
	return nbuf_put_int(&o->o, 8, 2, v);
}

static inline uint8_t
Foo_prefix(const Foo *o)
{
	return nbuf_get_int(&o->o, 10, 1);
}

static inline void
Foo_set_prefix(const Foo *o, uint8_t v)
{
	return nbuf_put_int(&o->o, 10, 1, v);
}

static inline uint32_t
Foo_length(const Foo *o)
{
	return nbuf_get_int(&o->o, 12, 4);
}

static inline void
Foo_set_length(const Foo *o, uint32_t v)
{
	return nbuf_put_int(&o->o, 12, 4, v);
}

static inline Foo
Bar_parent(const Bar *o)
{
	Foo oo;
	oo.o = nbuf_get_ptr(&o->o, 0);
	return oo;
}

static inline Foo
Bar_init_parent(const Bar *o)
{
	struct nbuf_obj oo = nbuf_create(o->o.buf, 1, 16, 0);
	nbuf_put_ptr(&o->o, 0, oo);
	Foo t = {oo};
	return t;}

static inline bool
Bar_has_parent(const Bar *o)
{
	return nbuf_has_ptr(&o->o, 0);
}

static inline int32_t
Bar_time(const Bar *o)
{
	return nbuf_get_int(&o->o, 0, 4);
}

static inline void
Bar_set_time(const Bar *o, int32_t v)
{
	return nbuf_put_int(&o->o, 0, 4, v);
}

static inline float
Bar_ratio(const Bar *o)
{
	return nbuf_get_f32(&o->o, 4);
}

static inline void
Bar_set_ratio(const Bar *o, float v)
{
	return nbuf_put_f32(&o->o, 4, v);
}

static inline uint16_t
Bar_size(const Bar *o)
{
	return nbuf_get_int(&o->o, 8, 2);
}

static inline void
Bar_set_size(const Bar *o, uint16_t v)
{
	return nbuf_put_int(&o->o, 8, 2, v);
}

static inline Bar
FooBar_sibling(const FooBar *o)
{
	Bar oo;
	oo.o = nbuf_get_ptr(&o->o, 0);
	return oo;
}

static inline Bar
FooBar_init_sibling(const FooBar *o)
{
	struct nbuf_obj oo = nbuf_create(o->o.buf, 1, 12, 1);
	nbuf_put_ptr(&o->o, 0, oo);
	Bar t = {oo};
	return t;}

static inline bool
FooBar_has_sibling(const FooBar *o)
{
	return nbuf_has_ptr(&o->o, 0);
}

static inline const char *
FooBar_name(const FooBar *o, size_t *lenp)
{
	return nbuf_get_str(&o->o, 1, lenp);
}

static inline void
FooBar_set_name(const FooBar *o, const char * v, size_t len)
{
	return nbuf_put_str(&o->o, 1, v, len);
}

static inline double
FooBar_rating(const FooBar *o)
{
	return nbuf_get_f64(&o->o, 0);
}

static inline void
FooBar_set_rating(const FooBar *o, double v)
{
	return nbuf_put_f64(&o->o, 0, v);
}

static inline uint8_t
FooBar_postfix(const FooBar *o)
{
	return nbuf_get_int(&o->o, 8, 1);
}

static inline void
FooBar_set_postfix(const FooBar *o, uint8_t v)
{
	return nbuf_put_int(&o->o, 8, 1, v);
}

static inline FooBar
FooBarContainer_list(const FooBarContainer *o, size_t i)
{
	FooBar oo;
	struct nbuf_obj t = nbuf_get_ptr(&o->o, 0);
	t = nbuf_get_elem(&t, i);
	oo.o = t;
	return oo;
}

static inline size_t
FooBarContainer_list_size(const FooBarContainer *o)
{
	struct nbuf_obj t = nbuf_get_ptr(&o->o, 0);
	return t.nelem;
}

static inline void
FooBarContainer_init_list(const FooBarContainer *o, size_t n)
{
	struct nbuf_obj oo = nbuf_create(o->o.buf, n, 12, 2);
	nbuf_put_ptr(&o->o, 0, oo);
}

static inline bool
FooBarContainer_has_list(const FooBarContainer *o)
{
	return nbuf_has_ptr(&o->o, 0);
}

static inline bool
FooBarContainer_initialized(const FooBarContainer *o)
{
	return nbuf_get_bit(&o->o, 0);
}

static inline void
FooBarContainer_set_initialized(const FooBarContainer *o, bool v)
{
	return nbuf_put_bit(&o->o, 0, v);
}

static inline Enum
FooBarContainer_fruit(const FooBarContainer *o)
{
	return nbuf_get_int(&o->o, 2, 2);
}

static inline void
FooBarContainer_set_fruit(const FooBarContainer *o, Enum v)
{
	return nbuf_put_int(&o->o, 2, 2, v);
}

static inline const char *
FooBarContainer_location(const FooBarContainer *o, size_t *lenp)
{
	return nbuf_get_str(&o->o, 1, lenp);
}

static inline void
FooBarContainer_set_location(const FooBarContainer *o, const char * v, size_t len)
{
	return nbuf_put_str(&o->o, 1, v, len);
}

#endif  /* BENCH_NB_H */
