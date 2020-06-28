/* Generate header from binary schema. */

#include "b2h_common.h"
#include "libnbufc.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

static void
genenums(struct nbuf_b2h *ctx, FILE *fout)
{
	size_t n = nbuf_Schema_enumTypes_size(ctx->schema);
	size_t i;

	for (i = 0; i < n; i++) {
		nbuf_EnumType e = nbuf_Schema_enumTypes(ctx->schema, i);
		const char *ename = nbuf_EnumType_name(e, NULL);
		size_t m = nbuf_EnumType_values_size(e);
		size_t j;

		fprintf(fout, "\ntypedef enum {\n");
		for (j = 0; j < m; j++) {
			nbuf_EnumDesc d = nbuf_EnumType_values(e, j);
			const char *name = nbuf_EnumDesc_name(d, NULL);
			uint16_t value = nbuf_EnumDesc_value(d);
			fprintf(fout, "\t%s%s_%s = %d,\n",
				ctx->prefix, ename, name, value);
		}
		fprintf(fout, "} %s%s;\n", ctx->prefix, ename);
	}
}

static void
gengetter(struct nbuf_b2h *ctx, FILE *fout,
	nbuf_FieldDesc fld, const char *mname)
{
	const char *fname = nbuf_FieldDesc_name(fld, NULL);
	nbuf_Kind kind = nbuf_FieldDesc_kind(fld);
	bool list = nbuf_FieldDesc_list(fld);
	uint32_t tag0 = nbuf_FieldDesc_tag0(fld);
	uint32_t tag1 = nbuf_FieldDesc_tag1(fld);
	const char *typ = typestr(ctx, kind, tag1);

	fprintf(fout, "\nstatic inline %s%s\n", maybeprefix(ctx, kind), typ); 
	fprintf(fout, "%s%s_%s(%s%s o%s%s)\n{\n",
		ctx->prefix, mname, fname, ctx->prefix, mname,
		list ? ", size_t i" : "",
		(kind == nbuf_Kind_STR) ? ", size_t *lenp" : ""); 
	if (list) {
		fprintf(fout, "\to.o = nbuf_get_ptr(&o.o, %u);\n", tag0);
		tag0 = 0;
		fprintf(fout, "\to.o = nbuf_get_elem(&o.o, i);\n");
	}
	switch (kind) {
	case nbuf_Kind_BOOL:
		fprintf(fout, "\treturn nbuf_get_bit(&o.o, ");
		if (!list)
			fprintf(fout, "%u);\n", tag0);
		else
			fprintf(fout, "i%%8);\n");
		break;
	case nbuf_Kind_INT:
	case nbuf_Kind_UINT:
		fprintf(fout, "\treturn nbuf_get_int(&o.o, %u, %u);\n", tag0, tag1);
		break;
	case nbuf_Kind_ENUM:
		fprintf(fout, "\treturn (%s%s) nbuf_get_int(&o.o, %u, 2);\n",
			ctx->prefix, typ, tag0);
		break;
	case nbuf_Kind_FLOAT:
		fprintf(fout, "\treturn nbuf_get_f%d(&o.o, %u);\n", tag1 * 8, tag0);
		break;
	case nbuf_Kind_STR:
		fprintf(fout, "\treturn nbuf_get_str(&o.o, %u, lenp);\n", tag0);
		break;
	case nbuf_Kind_PTR:
		fprintf(fout, "\treturn (%s%s) ", ctx->prefix, typ);
		if (!list) {
			fprintf(fout, "{nbuf_get_ptr(&o.o, %u)};\n", tag0);
		} else {
			fprintf(fout, "{o.o};\n");
		}
		break;
	default:
		assert(0 && "bad kind");
	}
	fprintf(fout, "}\n");

	// restore tag0 in case it was zeroed
	tag0 = nbuf_FieldDesc_tag0(fld);
	if (list) { // size getter
		fprintf(fout, "\nstatic inline size_t\n");
		fprintf(fout, "%s%s_%s_size(%s%s o)\n{\n",
			ctx->prefix, mname, fname, ctx->prefix, mname);
		fprintf(fout, "\treturn nbuf_get_ptr(&o.o, %u).nelem;\n", tag0);
		fprintf(fout, "}\n");
	}
}

static void
gensetter(struct nbuf_b2h *ctx, FILE *fout,
	nbuf_FieldDesc fld, const char *mname)
{
	const char *fname = nbuf_FieldDesc_name(fld, NULL);
	nbuf_Kind kind = nbuf_FieldDesc_kind(fld);
	bool list = nbuf_FieldDesc_list(fld);
	uint32_t tag0 = nbuf_FieldDesc_tag0(fld);
	uint32_t tag1 = nbuf_FieldDesc_tag1(fld);
	const char *typ = typestr(ctx, kind, tag1);

	if (kind == nbuf_Kind_PTR)
		return;  /* no setter */
	fprintf(fout, "\nstatic inline void\n");
	fprintf(fout, "%s%s_set_%s(%s%s o%s, %s%s v%s)\n{\n",
		ctx->prefix, mname, fname, ctx->prefix, mname,
		list ? ", size_t i" : "",
		maybeprefix(ctx, kind), typ,
		(kind == nbuf_Kind_STR) ? ", size_t len" : ""); 
	if (list) {
		fprintf(fout, "\to.o = nbuf_get_ptr(&o.o, %u);\n", tag0);
		tag0 = 0;
		fprintf(fout, "\to.o = nbuf_get_elem(&o.o, i);\n");
	}
	switch (kind) {
	case nbuf_Kind_BOOL:
		fprintf(fout, "\treturn nbuf_put_bit(&o.o, ");
		if (!list)
			fprintf(fout, "%u, v);\n", tag0);
		else
			fprintf(fout, "i%%8, v);\n");
		break;
	case nbuf_Kind_INT:
	case nbuf_Kind_UINT:
		fprintf(fout, "\treturn nbuf_put_int(&o.o, %u, %u, v);\n", tag0, tag1);
		break;
	case nbuf_Kind_ENUM:
		fprintf(fout, "\treturn nbuf_put_int(&o.o, %u, 2, (uint16_t) v);\n", tag0);
		break;
	case nbuf_Kind_FLOAT:
		fprintf(fout, "\treturn nbuf_put_f%d(&o.o, %u, v);\n", tag1 * 8, tag0);
		break;
	case nbuf_Kind_STR:
		fprintf(fout, "\treturn nbuf_put_str(&o.o, %u, v, len);\n", tag0);
		break;
	default:
		assert(0 && "bad kind");
	}
	fprintf(fout, "}\n");
}

static void
geniniter(struct nbuf_b2h *ctx, FILE *fout,
	nbuf_FieldDesc fld, const char *mname)
{
	const char *fname = nbuf_FieldDesc_name(fld, NULL);
	nbuf_Kind kind = nbuf_FieldDesc_kind(fld);
	bool list = nbuf_FieldDesc_list(fld);
	uint32_t tag0 = nbuf_FieldDesc_tag0(fld);
	uint32_t tag1 = nbuf_FieldDesc_tag1(fld);
	const char *typ = typestr(ctx, kind, tag1);
	struct nbuf_obj o;

	if (kind != nbuf_Kind_PTR && !list)
		return;
	fprintf(fout, "\nstatic inline ");
	if (list)
		fprintf(fout, "void");
	else
		fprintf(fout, "%s%s", ctx->prefix, typ);
	fprintf(fout, "\n%s%s_init_%s(%s%s o%s)\n{\n",
		ctx->prefix, mname, fname, ctx->prefix, mname,
		list ? ", size_t n" : "");
	o = getsizeinfo(ctx, kind, tag1);
	fprintf(fout, "\tstruct nbuf_obj oo = "
		"nbuf_create(o.o.buf, %s, %u, %u);\n",
		list ? "n" : "1", o.ssize, o.psize);
	fprintf(fout, "\tnbuf_put_ptr(&o.o, %u, oo);\n", tag0);
	if (!list)
		fprintf(fout, "\treturn (%s%s) {oo};\n", ctx->prefix, typ);
	fprintf(fout, "}\n");
}

static void
genhasfld(struct nbuf_b2h *ctx, FILE *fout,
	nbuf_FieldDesc fld, const char *mname)
{
	const char *fname = nbuf_FieldDesc_name(fld, NULL);
	nbuf_Kind kind = nbuf_FieldDesc_kind(fld);
	bool list = nbuf_FieldDesc_list(fld);
	uint32_t tag0 = nbuf_FieldDesc_tag0(fld);

	if (kind != nbuf_Kind_PTR && !list)
		return;
	fprintf(fout, "\nstatic inline bool\n");
	fprintf(fout, "%s%s_has_%s(%s%s o)\n{\n",
		ctx->prefix, mname, fname, ctx->prefix, mname);
	fprintf(fout, "\treturn nbuf_has_ptr(&o.o, %u);\n", tag0);
	fprintf(fout, "}\n");
}

static void
genmsgs(struct nbuf_b2h *ctx, FILE *fout)
{
	size_t n = nbuf_Schema_msgTypes_size(ctx->schema);
	size_t i;

	/* 1st pass - declare all types */
	for (i = 0; i < n; i++) {
		nbuf_MsgType m = nbuf_Schema_msgTypes(ctx->schema, i);
		const char *mname = nbuf_MsgType_name(m, NULL);
		uint16_t ssize = nbuf_MsgType_ssize(m);
		uint16_t psize = nbuf_MsgType_psize(m);

		/* struct */
		fprintf(fout, "\ntypedef struct {\n"
			"\tstruct nbuf_obj o;\n} %s%s;\n",
			ctx->prefix, mname);

		/* reader */
		fprintf(fout, "\nstatic inline %s%s\n", ctx->prefix, mname);
		fprintf(fout, "%sget_%s(struct nbuf_buffer *buf)\n",
			ctx->prefix, mname);
		fprintf(fout, "{\n\t%s%s o = {{buf}};\n"
			"\tnbuf_obj_init(&o.o, 0);\n\treturn o;\n}\n",
			ctx->prefix, mname);

		/* writer */
		fprintf(fout, "\nstatic inline %s%s\n", ctx->prefix, mname);
		fprintf(fout, "%snew_%s(struct nbuf_buffer *buf)\n",
			ctx->prefix, mname);
		fprintf(fout, "{\n\t%s%s o;\n"
			"\to.o = nbuf_create(buf, 1, %u, %u);\n\treturn o;\n}\n",
			ctx->prefix, mname, ssize, psize);
	}
	/* 2st pass - declare all accessors */
	for (i = 0; i < n; i++) {
		nbuf_MsgType msg = nbuf_Schema_msgTypes(ctx->schema, i);
		const char *mname = nbuf_MsgType_name(msg, NULL);
		size_t j, m = nbuf_MsgType_fields_size(msg);
		for (j = 0; j < m; j++) {
			nbuf_FieldDesc fld = nbuf_MsgType_fields(msg, j);
			gengetter(ctx, fout, fld, mname);
			gensetter(ctx, fout, fld, mname);
			geniniter(ctx, fout, fld, mname);
			genhasfld(ctx, fout, fld, mname);
		}
	}
}

static void
outhdr(struct nbuf_b2h *ctx, FILE *fout, const char *srcname)
{
	fprintf(fout, "/* Generated from %s.  DO NOT EDIT. */\n\n", srcname);
	fprintf(fout, "#ifndef %s\n#define %s\n\n", ctx->guard, ctx->guard);
	fprintf(fout, "#include \"nbuf.h\"\n");
	fprintf(fout, "#include \"nbuf.nb.h\"\n");
}

static void
outftr(struct nbuf_b2h *ctx, FILE *fout)
{
	size_t n;
	size_t i;

	/* These have to be put at the end to avoid forward declaration
	 * problems in nbuf.nb.h */
	n = nbuf_Schema_enumTypes_size(ctx->schema);
	if (n > 0)
		fprintf(fout, "\nextern nbuf_EnumType\n");
	for (i = 0; i < n; i++) {
		nbuf_EnumType e = nbuf_Schema_enumTypes(ctx->schema, i);
		const char *ename = nbuf_EnumType_name(e, NULL);
		fprintf(fout, "%srefl_%s%c\n",
			ctx->prefix, ename, (i == n-1) ? ';' : ',');
	}
	n = nbuf_Schema_msgTypes_size(ctx->schema);
	if (n > 0)
		fprintf(fout, "\nextern nbuf_MsgType\n");
	for (i = 0; i < n; i++) {
		nbuf_MsgType m = nbuf_Schema_msgTypes(ctx->schema, i);
		const char *mname = nbuf_MsgType_name(m, NULL);
		fprintf(fout, "%srefl_%s%c\n",
			ctx->prefix, mname, (i == n-1) ? ';' : ',');
	}
	fprintf(fout, "\nextern nbuf_Schema %srefl_schema;\n", ctx->prefix);
	fprintf(fout, "\n#endif  /* %s */\n", ctx->guard);
}

static void
cleanup(struct nbuf_b2h *ctx)
{
	free(ctx->prefix);
	free(ctx->guard);
}

void
nbuf_b2h(struct nbuf_buffer *buf, FILE *fout, const char *srcname)
{
	struct nbuf_b2h ctx[1];

	ctx->schema = nbuf_get_Schema(buf);
	makeprefix(ctx, srcname, "_H");
	outhdr(ctx, fout, srcname);
	genenums(ctx, fout);
	genmsgs(ctx, fout);
	outftr(ctx, fout);
	cleanup(ctx);
}
