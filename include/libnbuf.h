#ifndef LIBNBUF_H
#define LIBNBUF_H

#include "nbuf.h"
#include "nbuf.nb.h"

#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

extern const char *
nbuf_enum_name(nbuf_EnumType enm, uint16_t val);
extern uint32_t
nbuf_enum_value(nbuf_EnumType enm, const char *name);

extern bool
nbuf_find_msg(nbuf_MsgType *msg, nbuf_Schema schema, const char *name);

extern bool
nbuf_find_enum(nbuf_EnumType *enm, nbuf_Schema schema, const char *name);

extern bool
nbuf_find_field(nbuf_FieldDesc *fld, nbuf_MsgType msg, const char *name);

/* fills in ssize and psize */
extern void
_nbuf_get_size(struct nbuf_obj *o, nbuf_Schema schema, nbuf_Kind kind, uint32_t tag1);

static inline void
nbuf_get_size(struct nbuf_obj *o, nbuf_Schema schema, nbuf_FieldDesc fld)
{
	_nbuf_get_size(o, schema, nbuf_FieldDesc_kind(fld), nbuf_FieldDesc_tag1(fld));
}

/* reference to a field */
struct nbuf_ref {
	struct nbuf_obj o;
	nbuf_Kind kind;
	bool list;
	uint32_t tag0, tag1;
};

static inline struct nbuf_ref
nbuf_get_ref(const struct nbuf_obj *o, nbuf_FieldDesc fld)
{
	struct nbuf_ref ref;
	ref.o = *o;
	ref.kind = nbuf_FieldDesc_kind(fld);
	ref.list = nbuf_FieldDesc_list(fld);
	ref.tag0 = nbuf_FieldDesc_tag0(fld);
	ref.tag1 = nbuf_FieldDesc_tag1(fld);
	return ref;
}

static inline int64_t
nbuf_ref_get_int(struct nbuf_ref ref)
{
	uint32_t sz = (ref.kind == nbuf_Kind_ENUM) ? 2 : ref.tag1;
	int64_t v;
	assert(!ref.list);
	assert(ref.kind == nbuf_Kind_INT || ref.kind == nbuf_Kind_UINT || ref.kind == nbuf_Kind_ENUM);
	v = nbuf_get_int(&ref.o, ref.tag0, sz);
	if (ref.kind == nbuf_Kind_UINT)
		v &= ~(uint64_t) 0 >> (8-sz) * 8;
	return v;
}

static inline void
nbuf_ref_put_int(struct nbuf_ref ref, int64_t v)
{
	uint32_t sz = (ref.kind == nbuf_Kind_ENUM) ? 2 : ref.tag1;
	assert(!ref.list);
	assert(ref.kind == nbuf_Kind_INT || ref.kind == nbuf_Kind_UINT || ref.kind == nbuf_Kind_ENUM);
	nbuf_put_int(&ref.o, ref.tag0, sz, v);
}

static inline double
nbuf_ref_get_float(struct nbuf_ref ref)
{
	assert(!ref.list);
	assert(ref.kind == nbuf_Kind_FLOAT);
	switch (ref.tag1) {
	case 4: return nbuf_get_f32(&ref.o, ref.tag0);
	case 8: return nbuf_get_f64(&ref.o, ref.tag0);
	default: assert(0 && "unknown float size");
	}
	return 0;
}

static inline void
nbuf_ref_put_float(struct nbuf_ref ref, double v)
{
	assert(!ref.list);
	assert(ref.kind == nbuf_Kind_FLOAT);
	switch (ref.tag1) {
	case 4: nbuf_put_f32(&ref.o, ref.tag0, v); break;
	case 8: nbuf_put_f64(&ref.o, ref.tag0, v); break;
	default: assert(0 && "unknown float size");
	}
}

static inline bool
nbuf_ref_get_bit(struct nbuf_ref ref)
{
	assert(!ref.list);
	assert(ref.kind == nbuf_Kind_BOOL);
	return nbuf_get_bit(&ref.o, ref.tag0);
}

static inline void
nbuf_ref_put_bit(struct nbuf_ref ref, bool v)
{
	assert(!ref.list);
	assert(ref.kind == nbuf_Kind_BOOL);
	return nbuf_put_bit(&ref.o, ref.tag0, v);
}

static inline const char *
nbuf_ref_get_str(struct nbuf_ref ref, size_t *lenp)
{
	assert(!ref.list);
	assert(ref.kind == nbuf_Kind_STR);
	return nbuf_get_str(&ref.o, ref.tag0, lenp);
}

static inline void
nbuf_ref_put_str(struct nbuf_ref ref, const char *v, size_t len)
{
	assert(!ref.list);
	assert(ref.kind == nbuf_Kind_STR);
	return nbuf_put_str(&ref.o, ref.tag0, v, len);
}

static inline void
nbuf_ref_get_ptr(struct nbuf_ref *ref)
{
	assert(ref->list || ref->kind == nbuf_Kind_PTR);
	ref->o = nbuf_get_ptr(&ref->o, ref->tag0);
	ref->tag0 = 0;
}

static inline bool
nbuf_ref_advance(struct nbuf_ref *ref, size_t n)
{
	assert(ref->list);
	if (ref->o.nelem <= n)
		return false;
	ref->o.nelem -= n;
	if (ref->kind == nbuf_Kind_BOOL) {
		size_t m = ref->tag0 + n;
		ref->o.base += m / 8;
		ref->tag0 = m % 8;
	} else {
		ref->o.base += n * nbuf_elemsz(&ref->o);
	}
	return true;
}

/* initialize object/list or append to the list */
static inline void
nbuf_ref_init_ptr(struct nbuf_ref *ref, nbuf_Schema schema)
{
	struct nbuf_obj list;
	if (nbuf_has_ptr(&ref->o, ref->tag0)) {
		size_t n;
		assert(ref->list);
		list = nbuf_get_ptr(&ref->o, ref->tag0);
		n = list.nelem;
		nbuf_resize(&list, n + 1);
		ref->o = nbuf_get_elem(&list, n);
		ref->tag0 = (ref->kind == nbuf_Kind_BOOL) ? n % 8 : 0;
	} else {
		assert(ref->list || ref->kind == nbuf_Kind_PTR);
		_nbuf_get_size(&list, schema, ref->kind, ref->tag1);
		list = nbuf_create(ref->o.buf, 1, list.ssize, list.psize);
		nbuf_put_ptr(&ref->o, ref->tag0, list);
		ref->o = list;
		ref->tag0 = 0;
	}
	ref->list = false;
}

extern int
nbuf_print(const struct nbuf_obj *o, FILE *fout, int indent,
	nbuf_Schema schema, nbuf_MsgType msgType);

extern int
nbuf_raw_print(const struct nbuf_obj *o, FILE *fout, int indent);

extern bool
nbuf_load_file(struct nbuf_buffer *buf, FILE *fin);

extern bool
nbuf_save_file(struct nbuf_buffer *buf, FILE *fout);

/* Builder API
 *
 * Each object has its own buffer, so it can always be resized.
 * This property makes it useful to create arrays whose size is not
 * known in advance.
 *
 */

extern bool nbuf_init_builder(struct nbuf_buffer *, size_t);
extern void nbuf_free_builder(struct nbuf_buffer *);
extern bool nbuf_serialize(struct nbuf_buffer *);

/* parser */

typedef enum {
	nbuf_Token_UNK = 256,
	nbuf_Token_ID,
	nbuf_Token_INT,
	nbuf_Token_FLT,
	nbuf_Token_STR,
} nbuf_Token;

struct nbuf_lexer {
	FILE *fin;
	int lineno;
	void (*error)(struct nbuf_lexer *l, const char *fmt, ...);
	union {
		int64_t i;
		uint64_t u;
		double f;
		struct nbuf_buffer s;
	} u;
};

extern void
nbuf_lex_init(struct nbuf_lexer *l, FILE *fin);

extern nbuf_Token
nbuf_lex(struct nbuf_lexer *l);

extern bool
nbuf_parse(struct nbuf_buffer *buf, struct nbuf_lexer *l,
	nbuf_Schema schema, nbuf_MsgType msgType);

#ifdef __cplusplus
}
#endif

#endif  /* LIBNBUF_H */
