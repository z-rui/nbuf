#include "libnbuf.h"

#include <stdio.h>
#include <string.h>

struct nbuf_parser {
	nbuf_Token token;
	struct nbuf_pool pool;
	struct nbuf_lexer *l;
	nbuf_Schema schema;
};

struct ref {
	struct nbuf_obj o;
	nbuf_Kind kind;
	unsigned offs, sz, tag1;
};

static bool
getref(struct ref *ref,
	struct nbuf_parser *p,
	struct nbuf_obj o,
	nbuf_MsgType msg,
	const char *name)
{
	nbuf_FieldDesc fld;
	if (!nbuf_MsgType_fields_by_name(&fld, msg, name)) {
		p->l->error(p->l, "message %s has no field named \"%s\"",
			nbuf_MsgType_name(msg, NULL), name);
		return false;
	}
	ref->kind = nbuf_FieldDesc_kind(fld);
	unsigned tag0 = nbuf_FieldDesc_tag0(fld);
	ref->tag1 = nbuf_FieldDesc_tag1(fld);
	size_t ssize = 0, psize = 0;
	switch (ref->kind) {
	case nbuf_Kind_ENUM:
		ssize = 2;
		break;
	case nbuf_Kind_BOOL:
	case nbuf_Kind_INT:
	case nbuf_Kind_UINT:
	case nbuf_Kind_FLOAT:
		ssize = ref->tag1;
		break;
	case nbuf_Kind_STR:
		psize = 1;
		break;
	case nbuf_Kind_PTR: {
		nbuf_MsgType msg = nbuf_Schema_msgTypes(p->schema, ref->tag1);
		ssize = nbuf_MsgType_ssize(msg);
		psize = nbuf_MsgType_psize(msg);
		break;
	}
	}
	bool list = nbuf_FieldDesc_list(fld);
	if (list || ref->kind == nbuf_Kind_PTR) {
		size_t base = o.base + nbuf_ptr_offs(&o, tag0);
		if (nbuf_read_int_safe(o.buf, base, NBUF_WORD_SZ) != 0) {
			o = nbuf_get_pool_ptr(&p->pool, &o, tag0);
		} else {
			struct nbuf_obj oo = nbuf_pool_create(&p->pool,
				(list) ? 0 : 1, ssize, psize);
			nbuf_put_pool_ptr(&p->pool, &o, tag0, oo);
			o = oo;
		}
		ref->offs = 0;
		if (list) {
			size_t n = o.nelem;
			nbuf_resize(&o, n + 1);
			o = nbuf_get_elem(&o, n);
			if (ref->kind == nbuf_Kind_BOOL)
				ref->offs = n % 8;
		}
	} else {
		ref->offs = tag0;
	}
	ref->sz = ssize;
	ref->o = o;
	return true;
}

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
parse_bool(struct ref *ref, struct nbuf_parser *p, const char *field_name)
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
	nbuf_put_bit(&ref->o, ref->offs, v);
	free_tok(p);
	NEXT;
	return true;
}

static bool
parse_enum(struct ref *ref, struct nbuf_parser *p, const char *field_name)
{
	struct nbuf_lexer *l = p->l;
	uint32_t v;

	switch (p->token) {
	case nbuf_Token_INT:
		v = l->u.i;
		break;
	case nbuf_Token_ID: {
		nbuf_EnumType etype = nbuf_Schema_enumTypes(
			p->schema, ref->tag1);
		v = nbuf_EnumType_name_to_value(etype, l->u.s.base);
		if (v == (uint32_t) -1) {
			l->error(l, "enum %s has no value %s",
				nbuf_EnumType_name(etype, NULL),
				l->u.s.base);
			return false;
		}
		break;
	}
	default:
		l->error(l, "expecting enum value for \"%s\"", field_name);
		return false;
	}
	nbuf_put_int(&ref->o, ref->offs, 2, v);
	free_tok(p);
	NEXT;
	return true;
}

static bool
parse_int(struct ref *ref, struct nbuf_parser *p, const char *field_name)
{
	struct nbuf_lexer *l = p->l;
	if (p->token != nbuf_Token_INT) {
		l->error(l, "expecting integer for \"%s\"", field_name);
		return false;
	}
	nbuf_put_int(&ref->o, ref->offs, ref->sz, l->u.i);
	free_tok(p);
	NEXT;
	return true;
}

static bool
parse_float(struct ref *ref, struct nbuf_parser *p, const char *field_name)
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
	switch (ref->sz) {
	case 4: nbuf_put_f32(&ref->o, ref->offs, v); break;
	case 8: nbuf_put_f64(&ref->o, ref->offs, v); break;
	default: assert(0 && "unknown float size"); return false;
	}
	free_tok(p);
	NEXT;
	return true;
}

static bool
parse_str(struct ref *ref, struct nbuf_parser *p, const char *field_name)
{
	size_t len = p->l->u.s.len;
	struct nbuf_obj o = nbuf_pool_create(&p->pool, len, 1, 0);
	memcpy(o.buf->base + o.base, p->l->u.s.base, len);
	nbuf_put_pool_ptr(&p->pool, &ref->o, ref->offs, o);
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
		struct ref ref;
		field_name = steal(&p->l->u.s);
		if (!(ok = getref(&ref, p, *o, msgType, field_name)))
			goto out;
		NEXT;
		if (ref.kind != nbuf_Kind_PTR)
			EXPECT(':', "missing ':' after \"%s\"", field_name);
		else
			OPTIONAL(':');
		switch (ref.kind) {
#define PARSE(typ) if (!(ok = parse_##typ(&ref, p, field_name))) goto out
		case nbuf_Kind_BOOL: PARSE(bool); break;
		case nbuf_Kind_ENUM: PARSE(enum); break;
		case nbuf_Kind_INT: case nbuf_Kind_UINT: PARSE(int); break;
		case nbuf_Kind_FLOAT: PARSE(float); break;
		case nbuf_Kind_STR: PARSE(str); break;
#undef PARSE
		case nbuf_Kind_PTR: {
			nbuf_MsgType subMsgType;
			EXPECT('{', "missing '{' for \"%s\"", field_name);
			subMsgType = nbuf_Schema_msgTypes(p->schema, ref.tag1);
			if (!(ok = parse_msg(&ref.o, p, subMsgType)))
				goto out;
			EXPECT('}', "missing '}' for \"%s\"", field_name);
			break;
		}
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
	o = nbuf_pool_create(&p->pool, 1,
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
	nbuf_pool_serialize(buf, &p->pool);
	nbuf_pool_free(&p->pool);
	return ok;
}

