#ifndef LIBNBUF_H
#define LIBNBUF_H

#include "nbuf.h"
#include "nbuf.nb.h"

#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

extern const char *
nbuf_EnumType_value_to_name(nbuf_EnumType enm, uint16_t val);
extern uint32_t
nbuf_EnumType_name_to_value(nbuf_EnumType enm, const char *name);

extern bool
nbuf_Schema_msgType_by_name(nbuf_MsgType *msg, nbuf_Schema schema, const char *name);

extern bool
nbuf_Schema_enumType_by_name(nbuf_EnumType *enm, nbuf_Schema schema, const char *name);

extern bool
nbuf_MsgType_fields_by_name(nbuf_FieldDesc *fld, nbuf_MsgType msg, const char *name);

extern int
nbuf_print(struct nbuf_obj *o, FILE *fout, int indent,
	nbuf_Schema schema, nbuf_MsgType *msgType);

extern bool
nbuf_load_file(struct nbuf_buffer *buf, FILE *fin);

extern bool
nbuf_save_file(struct nbuf_buffer *buf, FILE *fout);

/* "Scaffolding" API
 *
 * Each object has its own buffer, so it can always be resized.
 * This property makes it useful to create arrays whose size is not
 * known in advance.
 *
 * The pointer representation is different: it is the index to an global
 * array of buffers---the "pool".
 *
 * A scaffolding object can be converted to a normal object.
 * During conversion, child objects are considered scaffolding objects
 * (and converted) as well.
 */
struct nbuf_pool {
	struct nbuf_buffer **buffers;
	uint32_t len, cap;
};

extern struct nbuf_obj
nbuf_pool_create(struct nbuf_pool *pool,
	uint32_t nelem,
	uint16_t ssize,
	uint16_t psize);

extern struct nbuf_obj
nbuf_get_pool_ptr(const struct nbuf_pool *pool,
	const struct nbuf_obj *r, size_t i);

extern void
nbuf_put_pool_ptr(struct nbuf_pool *pool,
	struct nbuf_obj *r, size_t i, struct nbuf_obj v);

extern struct nbuf_obj
nbuf_pool_serialize(struct nbuf_buffer *buf, const struct nbuf_pool *pool);

extern void
nbuf_pool_free(struct nbuf_pool *pool);

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
