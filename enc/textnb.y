%{
#include "textnb.tab.h"
#include "libnbuf.h"

#include <assert.h>
#include <inttypes.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern int yylex();
extern int yylineno;
static void yyerror(const char *fmt, ...);

extern nbuf_Schema schema;
extern nbuf_MsgType rootType;
extern struct nbuf_buffer buf;

struct ctx {
	struct nbuf_obj o;
	nbuf_MsgType msg;
	struct ctx *prev;
};

static struct ctx *ctx_top = NULL;
static void push_ctx(struct nbuf_obj o, nbuf_MsgType msg);
static void pop_ctx(void);

static struct nbuf_pool pool = {NULL};

struct ref {
	struct nbuf_obj o;
	nbuf_Kind kind;
	unsigned offs, sz, tag1;
};

static bool getref(struct ref *ref, const char *name);

%}

%code requires {
#include "nbuf.h"
#include <stdint.h>
}

%define parse.error verbose 

%union {
	int64_t i;
	double f;
	char *s;
}

%token <i> INT
%token <f> FLOAT
%token <s> STRING ID

%destructor { free($$); } <s>

%%

input
:
{
	struct nbuf_obj o;

	o = nbuf_pool_create(&pool, 1,
		nbuf_MsgType_ssize(&rootType),
		nbuf_MsgType_psize(&rootType));
	push_ctx(o, rootType);
}
msg_body
{
	nbuf_pool_serialize(&buf, &pool);
	nbuf_pool_free(&pool);
	pop_ctx();
	assert(ctx_top == NULL);
}
;

msg_body
:	/* empty */
|	msg_body ID ':' INT
{
	int64_t v = $4;
	struct ref ref;
	if (!getref(&ref, $2))
		goto out1;
	switch (ref.kind) {
	case nbuf_Kind_BOOL:
		nbuf_put_bit(&ref.o, ref.offs, v);
		break;
	case nbuf_Kind_ENUM: {
		nbuf_EnumType etype = nbuf_Schema_enumTypes(&schema, ref.tag1);
		if (nbuf_EnumType_value_to_name(&etype, v) == NULL) {
			yyerror("enum %s has no value %" PRId64,
				nbuf_EnumType_name(&etype, NULL), v);
			v = 0;
		}
		nbuf_put_int(&ref.o, ref.offs, 2, v);
		break;
	}
	case nbuf_Kind_INT: case nbuf_Kind_UINT:
		nbuf_put_int(&ref.o, ref.offs, ref.sz, v);
		break;
	case nbuf_Kind_FLOAT:
		if (ref.sz == 4)
			nbuf_put_f32(&ref.o, ref.offs, v);
		else if (ref.sz == 8)
			nbuf_put_f64(&ref.o, ref.offs, v);
		else
			assert(0 && "unknown float size");
		break;
	default:
		yyerror("cannot assign integer to \"%s\"", $2);
		break;
	}
out1:
	free($2);
}
|	msg_body ID ':' FLOAT
{
	struct ref ref;
	if (!getref(&ref, $2))
		goto out2;
	switch (ref.kind) {
	case nbuf_Kind_FLOAT:
		if (ref.sz == 4)
			nbuf_put_f32(&ref.o, ref.offs, $4);
		else if (ref.sz == 8)
			nbuf_put_f64(&ref.o, ref.offs, $4);
		else
			assert(0 && "unknown float size");
		break;
	default:
		yyerror("cannot assign float to \"%s\"", $2);
		break;
	}
out2:
	free($2);
}
|	msg_body ID ':' STRING
{
	struct ref ref;
	if (!getref(&ref, $2))
		goto out3;
	switch (ref.kind) {
	case nbuf_Kind_STR: {
		size_t len = strlen($4);
		struct nbuf_obj o = nbuf_pool_create(&pool,
			len+1, 1, 0);
		memcpy(o.buf->base + o.base, $4, len+1);
		nbuf_put_pool_ptr(&pool, &ref.o, ref.offs, o);
		break;
	}
	default:
		yyerror("cannot assign string to \"%s\"", $2);
		break;
	}
out3:
	free($2);
	free($4);
}
|	msg_body ID ':' ID
{
	struct ref ref;
	if (!getref(&ref, $2))
		goto out4;
	switch (ref.kind) {
	case nbuf_Kind_BOOL: {
		bool v = false;
		if (strcmp($4, "true") == 0)
			v = true;
		else if (strcmp($4, "false") != 0)
			yyerror("syntax error, unexpected %s, "
				"expecting true or false", $4);
		nbuf_put_bit(&ref.o, ref.offs, v);
		break;
	}
	case nbuf_Kind_ENUM: {
		nbuf_EnumType etype = nbuf_Schema_enumTypes(&schema, ref.tag1);
		uint32_t v = nbuf_EnumType_name_to_value(&etype, $4);
		if (v == (uint32_t) -1) {
			yyerror("enum %s has no value %s",
				nbuf_EnumType_name(&etype, NULL), $4);
			v = 0;
		}
		nbuf_put_int(&ref.o, ref.offs, 2, v);
		break;
	}
	default:
		yyerror("cannot assign %s to \"%s\"", $4, $2);
		break;
	}
out4:
	free($2);
	free($4);
}
|	msg_body ID '{'
{
	struct ref ref;
	if (!getref(&ref, $2))
		YYERROR;
	switch (ref.kind) {
	case nbuf_Kind_PTR: {
		nbuf_MsgType mtype = nbuf_Schema_msgTypes(&schema, ref.tag1);
		push_ctx(ref.o, mtype);
		break;
	}
	default:
		yyerror("syntax error, unexpected '{', expecting ':'");
		YYERROR;
		break;
	}
}
	msg_body '}'
{
	free($2);
	pop_ctx();
}
;

%%

void yyerror(const char *fmt, ...)
{
	va_list argp;
	fprintf(stderr, "%d: ", yylineno);
	va_start(argp, fmt);
	vfprintf(stderr, fmt, argp);
	va_end(argp);
	fprintf(stderr, "\n");
}

void push_ctx(struct nbuf_obj o, nbuf_MsgType msg)
{
	struct ctx *ctx = malloc(sizeof *ctx);
	ctx->o = o;
	ctx->msg = msg;
	ctx->prev = ctx_top;
	ctx_top = ctx;
}

void pop_ctx(void)
{
	struct ctx *ctx = ctx_top;
	ctx_top = ctx->prev;
	free(ctx);
}

bool getref(struct ref *ref, const char *name)
{
	nbuf_FieldDesc fld;
	nbuf_MsgType *msg = &ctx_top->msg;
	bool ok = nbuf_MsgType_fields_by_name(&fld, msg, name);
	if (!ok) {
		yyerror("message %s has no field named \"%s\"",
			nbuf_MsgType_name(msg, NULL), name);
		return false;
	}
	ref->kind = nbuf_FieldDesc_kind(&fld);
	struct nbuf_obj o = ctx_top->o;
	unsigned tag0 = nbuf_FieldDesc_tag0(&fld);
	ref->tag1 = nbuf_FieldDesc_tag1(&fld);
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
		nbuf_MsgType msg = nbuf_Schema_msgTypes(&schema, ref->tag1);
		ssize = nbuf_MsgType_ssize(&msg);
		psize = nbuf_MsgType_psize(&msg);
		break;
	}
	}
	bool list = nbuf_FieldDesc_list(&fld);
	if (list || ref->kind == nbuf_Kind_PTR) {
		size_t base = o.base + nbuf_ptr_offs(&o, tag0);
		if (nbuf_read_int_safe(o.buf, base, NBUF_WORD_SZ) != 0) {
			o = nbuf_get_pool_ptr(&pool, &o, tag0);
		} else {
			struct nbuf_obj oo = nbuf_pool_create(&pool,
				(list) ? 0 : 1, ssize, psize);
			nbuf_put_pool_ptr(&pool, &o, tag0, oo);
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
