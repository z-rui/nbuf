#include "libnbuf.h"

#include <stdio.h>
#include <string.h>

struct nbuf_parser {
	nbuf_Token token;
	struct nbuf_buffer *buf;  /* builder buffer */
	struct nbuf_lexer *l;
	nbuf_Schema schema;
};

#define NEXT p->token = nbuf_lex(p->l)
#define OPTIONAL(tok) if (p->token == (tok)) NEXT;
#define EXPECT(tok, msg, ...) do { OPTIONAL(tok) else { \
	p->l->error(p->l, msg, ##__VA_ARGS__); \
	return false; \
} } while (0)

static char *
steal(struct nbuf_buffer *buf)
{
	char *s = buf->base;
	buf->base = NULL;
	buf->len = buf->cap = 0;
	return s;
}

static void
free_tok(struct nbuf_parser *p)
{
	switch (p->token) {
	case nbuf_Token_ID:
	case nbuf_Token_STR:
		nbuf_free(&p->l->u.s);
		break;
	default:
		break;
	}
}

static bool
parse_bool(struct nbuf_ref ref, struct nbuf_parser *p, const char *field_name)
{
	struct nbuf_lexer *l = p->l;
	bool v;

	switch (p->token) {
	case nbuf_Token_INT:
		v = l->u.i;
		break;
	case nbuf_Token_ID: {
		const char *s = l->u.s.base;
		if (strcmp(s, "true") == 0)
			v = true;
		else if (strcmp(s, "false") == 0)
			v = false;
		else {
	default:
			l->error(l, "expecting true or false for \"%s\"",
				field_name);
			return false;
		}
	}
	}
	nbuf_ref_put_bit(ref, v);
	free_tok(p);
	NEXT;
	return true;
}

static bool
parse_enum(struct nbuf_ref ref, struct nbuf_parser *p, const char *field_name)
{
	struct nbuf_lexer *l = p->l;
	uint32_t v;

	switch (p->token) {
	case nbuf_Token_INT:
		v = l->u.i;
		break;
	case nbuf_Token_ID: {
		nbuf_EnumType etype = nbuf_Schema_enumTypes(p->schema, ref.tag1);
		v = nbuf_enum_value(etype, l->u.s.base);
		if (v == (uint32_t) -1) {
			l->error(l, "enum %s has no value %s",
				nbuf_EnumType_name(etype, NULL),
				l->u.s.base);
			return false;
		}
		break;
	}
	default:
		l->error(l, "expecting enum value for \"%s\", got %s", field_name, p->token);
		return false;
	}
	nbuf_ref_put_int(ref, v);
	free_tok(p);
	NEXT;
	return true;
}

static bool
parse_int(struct nbuf_ref ref, struct nbuf_parser *p, const char *field_name)
{
	struct nbuf_lexer *l = p->l;
	if (p->token != nbuf_Token_INT) {
		l->error(l, "expecting integer for \"%s\"", field_name);
		return false;
	}
	nbuf_ref_put_int(ref, l->u.i);
	free_tok(p);
	NEXT;
	return true;
}

static bool
parse_float(struct nbuf_ref ref, struct nbuf_parser *p, const char *field_name)
{
	struct nbuf_lexer *l = p->l;
	double v;
	switch (p->token) {
	case nbuf_Token_INT: v = l->u.i; break;
	case nbuf_Token_FLT: v = l->u.f; break;
	default:
		l->error(l, "expecting number for \"%s\"", field_name);
		return false;
	}
	nbuf_ref_put_float(ref, v);
	free_tok(p);
	NEXT;
	return true;
}

static bool
parse_str(struct nbuf_ref ref, struct nbuf_parser *p, const char *field_name)
{
	struct nbuf_lexer *l = p->l;
	size_t len = l->u.s.len;
	assert(len > 0 && l->u.s.base[len-1] == '\0');
	nbuf_ref_put_str(ref, l->u.s.base, len-1);
	free_tok(p);
	NEXT;
	return true;
}

static bool
parse_msg(struct nbuf_obj *o, struct nbuf_parser *p, nbuf_MsgType msgType)
{
	char *field_name = NULL;
	bool ok = true;

	while (p->token == nbuf_Token_ID) {
		struct nbuf_ref ref;
		nbuf_FieldDesc fld;
		field_name = steal(&p->l->u.s);
		if (!nbuf_find_field(&fld, msgType, field_name)) {
			p->l->error(p->l, "message %s has no field named \"%s\"",
				nbuf_MsgType_name(msgType, NULL), field_name);
			goto out;
		}
		ref = nbuf_get_ref(o, fld);
		if (ref.kind == nbuf_Kind_PTR && !ref.list && nbuf_has_ptr(&ref.o, ref.tag0)) {
			p->l->error(p->l, "non-list field %s specified more than once",
				field_name);
			goto out;
		}
		if (ref.list || ref.kind == nbuf_Kind_PTR)
			nbuf_ref_init_ptr(&ref, p->schema);
		NEXT;
		if (ref.kind != nbuf_Kind_PTR)
			EXPECT(':', "missing ':' after \"%s\"", field_name);
		else
			OPTIONAL(':');
		switch (ref.kind) {
#define PARSE(typ) if (!(ok = parse_##typ(ref, p, field_name))) goto out
		case nbuf_Kind_BOOL: PARSE(bool); break;
		case nbuf_Kind_ENUM: PARSE(enum); break;
		case nbuf_Kind_INT: case nbuf_Kind_UINT: PARSE(int); break;
		case nbuf_Kind_FLOAT: PARSE(float); break;
		case nbuf_Kind_STR: PARSE(str); break;
#undef PARSE
		case nbuf_Kind_PTR:
			EXPECT('{', "missing '{' for \"%s\"", field_name);
			if (!(ok = parse_msg(&ref.o, p,
				nbuf_Schema_msgTypes(p->schema, ref.tag1))))
				goto out;
			EXPECT('}', "missing '}' for \"%s\"", field_name);
			break;
		}
		if (p->token == ',' || p->token == ';')
			NEXT;
		free(field_name);
		field_name = NULL;
	}
out:
	free(field_name);
	return ok;
}

bool
nbuf_parse(struct nbuf_buffer *buf, struct nbuf_lexer *l,
	nbuf_Schema schema, nbuf_MsgType msgType)
{
	struct nbuf_parser p[1];
	struct nbuf_obj o;
	bool ok;

	memset(p, 0, sizeof p);
	p->buf = buf;
	nbuf_init_builder(buf, 1);
	o = nbuf_create(buf, 1,
		nbuf_MsgType_ssize(msgType),
		nbuf_MsgType_psize(msgType));
	p->l = l;
	p->schema = schema;
	NEXT;
	ok = parse_msg(&o, p, msgType);
	if (ok && p->token != EOF) {
		l->error(l, "expecting end of file");
		ok = false;
		free_tok(p);
	}
	nbuf_serialize(buf);
	return ok;
}

