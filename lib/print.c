#include "libnbuf.h"

#include <ctype.h>
#include <float.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>

struct ctx {
	FILE *fout;
	nbuf_Schema schema;
	int indent;
	int indent_inc;
};

static int
do_indent(int n, FILE *fout)
{
	int i;

	if (n <= 0)
		return 0;
	for (i = 0; i < n; i++)
		fputc(' ', fout);
	return n;
}

static int
dumpstr(FILE *fout, const char *s, size_t n)
{
	int nwrite = 0;

	nwrite += fprintf(fout, "\"");
	while (n--) {
		unsigned char ch = *s++;
		nwrite +=
			(ch == '"') ? fprintf(fout, "\\\"") :
			(isprint(ch)) ? fprintf(fout, "%c", ch) :
			fprintf(fout, "\\x%02x", ch);
	}
	nwrite += fprintf(fout, "\"");
	return nwrite;
}

static int
do_print_notype(struct ctx *ctx, struct nbuf_obj *o)
{
	int nwrite = 0;
	char nl = (ctx->indent_inc >= 0) ? '\n' : ' ';
	uint32_t i, j;

	if (o->ssize > 0) {
		nwrite += do_indent(ctx->indent, ctx->fout);
		nwrite += fprintf(ctx->fout, "0: ");
		nwrite += dumpstr(ctx->fout, o->buf->base + o->base, o->ssize);
		nwrite += fprintf(ctx->fout, "%c", nl);
	}
	for (i = 0; i < o->psize; i++) {
		struct nbuf_obj oo;

		oo = nbuf_get_ptr(o, i);
		if (oo.ssize <= 1 && oo.psize == 0 && oo.nelem > 0) {
			oo.ssize = (oo.ssize == 0) ? (oo.nelem+7)/8 : oo.nelem;
			oo.nelem = 1;
		}
		for (j = 0; j < oo.nelem; j++) {
			if (!nbuf_bounds_check(oo.buf, oo.base, oo.ssize))
				break;  /* out of bounds */
			nwrite += do_indent(ctx->indent, ctx->fout);
			nwrite += fprintf(ctx->fout, "%" PRId32 ": ", i+1);
			if (oo.psize == 0) {
				nwrite += dumpstr(ctx->fout,
					oo.buf->base + oo.base,
					oo.ssize);
			} else {
				nwrite += fprintf(ctx->fout, "{%c", nl);
				ctx->indent += ctx->indent_inc;
				nwrite += do_print_notype(ctx, &oo);
				ctx->indent -= ctx->indent_inc;
				nwrite += do_indent(ctx->indent, ctx->fout);
				nwrite += fprintf(ctx->fout, "}");
			}
			nwrite += fprintf(ctx->fout, "%c", nl);
			oo.base += nbuf_elemsz(&oo);
		}
	}
	return nwrite;
}

static struct nbuf_obj
subobj(struct nbuf_obj *o, nbuf_FieldDesc fld)
{
	nbuf_Kind kind = nbuf_FieldDesc_kind(fld);
	unsigned tag0 = nbuf_FieldDesc_tag0(fld);
	unsigned tag1 = nbuf_FieldDesc_tag1(fld);
	struct nbuf_obj oo;

	oo.buf = o->buf;
	oo.nelem = 1;
	oo.ssize = tag1;
	oo.psize = 0;
	switch (kind) {
	case nbuf_Kind_BOOL:
		oo.base = o->base + tag0 / 8;
		oo.ssize = 1;
		break;
	case nbuf_Kind_STR:
	case nbuf_Kind_PTR:
		oo.base = o->base + nbuf_ptr_offs(o, tag0);
		oo.ssize = 0;
		oo.psize = 1;
		break;
	case nbuf_Kind_ENUM:
		oo.ssize = 2;
		/** fallthrough */
	default:
		oo.base = o->base + tag0;
		break;
	}
	if ((oo.ssize && oo.base >= o->base + o->ssize)
		|| (oo.psize && oo.base >= o->base + nbuf_elemsz(o)))
		oo.nelem = 0;
	return oo;
}

static int
do_print(struct ctx *ctx, struct nbuf_obj *o, nbuf_MsgType msgType)
{
	int nwrite = 0;
	size_t i, j, n;
	char nl = (ctx->indent_inc >= 0) ? '\n' : ' ';

	n = nbuf_MsgType_fields_size(msgType);
	for (i = 0; i < n; i++) {
		nbuf_FieldDesc fld;
		const char *fname, *s;
		unsigned tag0, tag1;
		nbuf_Kind kind;
		nbuf_MsgType fldMsgType;
		nbuf_EnumType fldEnumType;
		struct nbuf_obj oo;
		size_t len;

		fld = nbuf_MsgType_fields(msgType, i);
		fname = nbuf_FieldDesc_name(fld, NULL);
		kind = nbuf_FieldDesc_kind(fld);
		tag0 = nbuf_FieldDesc_tag0(fld);
		tag1 = nbuf_FieldDesc_tag1(fld);
		if (nbuf_FieldDesc_list(fld) || kind == nbuf_Kind_PTR) {
			oo = nbuf_get_ptr(o, tag0);
			if (kind == nbuf_Kind_BOOL)
				oo.ssize = 1;
			tag0 = 0;
		} else {
			oo = subobj(o, fld);
			if (kind == nbuf_Kind_BOOL)
				tag0 %= 8;
			else
				tag0 = 0;
		}
		if (kind == nbuf_Kind_ENUM)
			fldEnumType = nbuf_Schema_enumTypes(
				ctx->schema, tag1);
		else if (kind == nbuf_Kind_PTR)
			fldMsgType = nbuf_Schema_msgTypes(
				ctx->schema, tag1);

		for (j = 0; j < oo.nelem; j++) {
			if (!nbuf_bounds_check(oo.buf, oo.base, oo.ssize))
				break;  /* out of bounds */
			nwrite += do_indent(ctx->indent, ctx->fout);
			nwrite += fprintf(ctx->fout, "%s%s", fname,
				(kind == nbuf_Kind_PTR) ? " " : ": ");
			switch (kind) {
			case nbuf_Kind_BOOL:
				nwrite += fprintf(ctx->fout,
					"%s", nbuf_get_bit(&oo, tag0)
						? "true" : "false");
				break;
			case nbuf_Kind_INT:
				nwrite += fprintf(ctx->fout,
					"%" PRId64,
					nbuf_get_int(&oo, tag0, tag1));
				break;
			case nbuf_Kind_UINT:
				nwrite += fprintf(ctx->fout,
					"%" PRIu64,
					nbuf_get_int(&oo, tag0, tag1)
					& ((uint64_t) -1 >> (8*(8-tag1))));
				break;
			case nbuf_Kind_FLOAT:
				nwrite += (tag1 == 4)
					? fprintf(ctx->fout,
						"%.*g", FLT_DIG,
						nbuf_get_f32(&oo, tag0))
					: fprintf(ctx->fout,
						"%.*lg", DBL_DIG,
						nbuf_get_f64(&oo, tag0));
				break;
			case nbuf_Kind_STR:
				s = nbuf_get_str(&oo, 0, &len);
				nwrite += dumpstr(ctx->fout, s, len);
				break;
			case nbuf_Kind_ENUM:
				nwrite += fprintf(ctx->fout, "%s",
					nbuf_enum_name(fldEnumType, nbuf_get_int(&oo, tag0, 2)));
				break;
			case nbuf_Kind_PTR:
				nwrite += fprintf(ctx->fout, "{%c", nl);
				ctx->indent += ctx->indent_inc;
				nwrite += do_print(ctx, &oo, fldMsgType);
				ctx->indent -= ctx->indent_inc;
				nwrite += do_indent(ctx->indent, ctx->fout);
				nwrite += fprintf(ctx->fout, "}");
				break;
			}
			nwrite += fprintf(ctx->fout, "%c", nl);
			if (kind != nbuf_Kind_BOOL ||
					(tag0 = (tag0 + 1) % 8) == 0)
				oo.base += nbuf_elemsz(&oo);
		}
	}
	return nwrite;
}

int
nbuf_print(struct nbuf_obj *o, FILE *fout, int indent,
	nbuf_Schema schema, nbuf_MsgType *msgType)
{
	struct ctx ctx;

	ctx.fout = fout;
	ctx.schema = schema;
	ctx.indent = 0;
	ctx.indent_inc = indent;
	return msgType
		? do_print(&ctx, o, *msgType)
		: do_print_notype(&ctx, o);
}
