#include "nbuf.h"

/*
   enum Enum {
     Apples = 0
     Pears = 1
     Bananas = 2
   }
   message Foo {
     id: uint64  // 0
     count: uint16 // 8
     prefix: uint8 // 10
     length: uint32 // 12
   } // size = 16, 0
  
   message Bar {
     parent: Foo // p0
     time: int32 // 0
     ratio: float // 4
     size: uint16 // 8
   } // size = 12, 1
  
   message FooBar {
     sibling: Bar // p0
     name: string // p1
     rating: double // 0
     postfix: uint8 // 8
   } // size = 16, 2
  
   message FooBarContainer {
     list: []Foobar // p0
     initialized: bool // 0
     fruit: Enum // 2
     location: string // p1
   } // size = 4, 2
*/

typedef enum {
	Enum_Apples = 0,
	Enum_Pears = 1,
	Enum_Bananas = 2,
} Enum;

typedef struct {
	struct nbuf_obj o;
} Foo;

static inline uint64_t Foo_id(Foo o) { return nbuf_get_int(&o.o, 0, 8); }
static inline uint16_t Foo_count(Foo o) { return nbuf_get_int(&o.o, 8, 2); }
static inline uint8_t  Foo_prefix(Foo o) { return nbuf_get_int(&o.o, 10, 1); }
static inline uint32_t Foo_length(Foo o) { return nbuf_get_int(&o.o, 12, 4); }

static inline void Foo_set_id(Foo o, uint64_t v) { nbuf_put_int(&o.o, 0, 8, v); }
static inline void Foo_set_count(Foo o, uint16_t v) { nbuf_put_int(&o.o, 8, 2, v); }
static inline void Foo_set_prefix(Foo o, uint8_t v) { nbuf_put_int(&o.o, 10, 1, v); }
static inline void Foo_set_length(Foo o, uint32_t v) { nbuf_put_int(&o.o, 12, 4, v); }

typedef struct {
	struct nbuf_obj o;
} Bar;

static inline Foo Bar_parent(Bar o) {
	Foo oo = {nbuf_get_ptr(&o.o, 0)};
	return oo;
}

static inline int32_t  Bar_time(Bar o) { return nbuf_get_int(&o.o, 0, 4); }
static inline float    Bar_ratio(Bar o) { return nbuf_get_f32(&o.o, 4); }
static inline uint16_t Bar_size(Bar o) { return nbuf_get_int(&o.o, 8, 2); }

static inline Foo Bar_init_parent(Bar o) {
	Foo oo = {nbuf_create(o.o.buf, 1, 16, 0)};
	nbuf_put_ptr(&o.o, 0, oo.o);
	return oo;
}

static inline void Bar_set_time(Bar o, int32_t v) { nbuf_put_int(&o.o, 0, 4, v); }
static inline void Bar_set_ratio(Bar o, float v) { nbuf_put_f32(&o.o, 4, v); }
static inline void Bar_set_size(Bar o, int16_t v) { nbuf_put_int(&o.o, 8, 2, v); }

typedef struct {
	struct nbuf_obj o;
} FooBar;

static inline Bar FooBar_sibling(FooBar o) {
	Bar oo = {nbuf_get_ptr(&o.o, 0)};
	return oo;
}
static inline char *FooBar_name(FooBar o, size_t *lenp) {
	return nbuf_get_str(&o.o, 1, lenp);
}
static inline double FooBar_rating(FooBar o) { return nbuf_get_f64(&o.o, 0); }
static inline uint8_t FooBar_postfix(FooBar o) { return nbuf_get_int(&o.o, 8, 1); }

static inline Bar FooBar_init_sibling(FooBar o) {
	Bar oo = {nbuf_create(o.o.buf, 1, 12, 1)};
	nbuf_put_ptr(&o.o, 0, oo.o);
	return oo;
}

static inline void FooBar_set_name(FooBar o, const char *s, size_t len) { nbuf_put_str(&o.o, 1, s, len); }
static inline void FooBar_set_rating(FooBar o, double v) { nbuf_put_f64(&o.o, 0, v); }
static inline void FooBar_set_postfix(FooBar o, uint8_t v) { nbuf_put_int(&o.o, 8, 1, v); }

typedef struct {
	struct nbuf_obj o;
} FooBarContainer;

static inline FooBar FooBarContainer_list(FooBarContainer o, size_t i) {
	struct nbuf_obj t = nbuf_get_ptr(&o.o, 0);
	FooBar oo = {nbuf_get_elem(&t, i)};
	return oo;
}

static inline size_t FooBarContainer_list_size(FooBarContainer o) {
	return nbuf_get_ptr(&o.o, 0).nelem;
}

static inline bool FooBarContainer_initialized(FooBarContainer o) { return nbuf_get_bit(&o.o, 0); }
static inline Enum FooBarContainer_fruit(FooBarContainer o) { return (Enum) nbuf_get_int(&o.o, 2, 2); }
static inline char *FooBarContainer_location(FooBarContainer o, size_t *lenp) { return nbuf_get_str(&o.o, 1, lenp); }

static inline void FooBarContainer_init_list(FooBarContainer o, size_t nelem) {
	nbuf_put_ptr(&o.o, 0, nbuf_create(o.o.buf, nelem, 16, 2));
}
static inline void FooBarContainer_set_initialized(FooBarContainer o, bool v) {
	nbuf_put_bit(&o.o, 0, v);
}
static inline void FooBarContainer_set_fruit(FooBarContainer o, Enum v) {
	nbuf_put_int(&o.o, 2, 2, (int64_t) v);
}
static inline void FooBarContainer_set_location(FooBarContainer o, const char *s, size_t len) {
	nbuf_put_str(&o.o, 1, s, len);
}

static inline FooBarContainer nbuf_get_FooBarContainer(struct nbuf_buffer *buf)
{
	FooBarContainer o = {{buf}};
	nbuf_obj_init(&o.o, 0);
	return o;
}

static inline FooBarContainer nbuf_init_FooBarContainer(struct nbuf_buffer *buf)
{
	FooBarContainer o;
	o.o = nbuf_create(buf, 1, 4, 2);
	return o;
}
