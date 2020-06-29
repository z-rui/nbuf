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
do_print_notype(struct ctx *ctx, const struct nbuf_obj *o)
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

static int
do_print(struct ctx *ctx, const struct nbuf_obj *o, nbuf_MsgType msgType)
{
	int nwrite = 0;
	size_t i, n;
	char nl = (ctx->indent_inc >= 0) ? '\n' : ' ';

	n = nbuf_MsgType_fields_size(msgType);
	for (i = 0; i < n; i++) {
		nbuf_FieldDesc fld;
		struct nbuf_ref ref;
		const char *s;
		nbuf_MsgType fldMsgType;
		nbuf_EnumType fldEnumType;
		size_t len;

		fld = nbuf_MsgType_fields(msgType, i);
		ref = nbuf_get_ref(o, fld);
		if (ref.list || ref.kind == nbuf_Kind_PTR) {
			nbuf_ref_get_ptr(&ref);
			if (ref.o.nelem == 0)
				continue;
		}
		if (ref.kind == nbuf_Kind_ENUM)
			fldEnumType = nbuf_Schema_enumTypes(ctx->schema, ref.tag1);
		else if (ref.kind == nbuf_Kind_PTR)
			fldMsgType = nbuf_Schema_msgTypes(ctx->schema, ref.tag1);
		do {
			if (!nbuf_bounds_check(ref.o.buf, ref.o.base, ref.o.ssize))
				break;  /* out of bounds */
			nwrite += do_indent(ctx->indent, ctx->fout);
			nwrite += fprintf(ctx->fout, "%s%s",
				nbuf_FieldDesc_name(fld, NULL),
				(ref.kind == nbuf_Kind_PTR) ? " " : ": ");
			switch (ref.kind) {
			case nbuf_Kind_BOOL:
				nwrite += fprintf(ctx->fout,
					"%s", nbuf_ref_get_bit(ref) ? "true" : "false");
				break;
			case nbuf_Kind_INT:
				nwrite += fprintf(ctx->fout, "%" PRId64, nbuf_ref_get_int(ref));
				break;
			case nbuf_Kind_UINT:
				nwrite += fprintf(ctx->fout, "%" PRIu64, nbuf_ref_get_int(ref));
				break;
			case nbuf_Kind_FLOAT:
				nwrite += fprintf(ctx->fout, "%.*lg", DBL_DIG, nbuf_ref_get_float(ref));
				break;
			case nbuf_Kind_STR:
				s = nbuf_ref_get_str(ref, &len);
				nwrite += dumpstr(ctx->fout, s, len);
				break;
			case nbuf_Kind_ENUM:
				nwrite += fprintf(ctx->fout, "%s",
					nbuf_enum_name(fldEnumType, nbuf_ref_get_int(ref)));
				break;
			case nbuf_Kind_PTR:
				nwrite += fprintf(ctx->fout, "{%c", nl);
				ctx->indent += ctx->indent_inc;
				nwrite += do_print(ctx, &ref.o, fldMsgType);
				ctx->indent -= ctx->indent_inc;
				nwrite += do_indent(ctx->indent, ctx->fout);
				nwrite += fprintf(ctx->fout, "}");
				break;
			}
			nwrite += fprintf(ctx->fout, "%c", nl);
		} while (ref.list && nbuf_ref_advance(&ref, 1));
	}
	return nwrite;
}

int
nbuf_print(const struct nbuf_obj *o, FILE *fout, int indent,
	nbuf_Schema schema, nbuf_MsgType msgType)
{
	struct ctx ctx;

	ctx.fout = fout;
	ctx.schema = schema;
	ctx.indent = 0;
	ctx.indent_inc = indent;
	return do_print(&ctx, o, msgType);
}

int
nbuf_raw_print(const struct nbuf_obj *o, FILE *fout, int indent)
{
	struct ctx ctx;

	ctx.fout = fout;
	ctx.indent = 0;
	ctx.indent_inc = indent;
	return do_print_notype(&ctx, o);
}
