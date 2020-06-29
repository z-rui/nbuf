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
