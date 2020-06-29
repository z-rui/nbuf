/* Generate C++ header from binary schema. */

#include "b2h_common.h"
#include "libnbuf.h"
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

		fprintf(fout, "\nenum class %s {\n", ename);
		for (j = 0; j < m; j++) {
			nbuf_EnumDesc d = nbuf_EnumType_values(e, j);
			const char *name = nbuf_EnumDesc_name(d, NULL);
			uint16_t value = nbuf_EnumDesc_value(d);
			fprintf(fout, "\t%s = %d,\n", name, value);
		}
		fprintf(fout, "};\n");
	}
}

static void
outfnhdr(FILE *fout, const char *typ,
	const char *mname,
	const char *prefix,
	const char *fname,
	bool definition)
{
	/* output is
	 *     {typ}
	 *     {mname}::{prefix}{fname}
	 * if definition == true.
	 * otherwise it's
	 *     \tinline {typ} {prefix}{fname}
	 */
	fprintf(fout, "%s%s%s",
		definition ? "\n" : "\tinline ",
		typ,
		definition ? "\n" : " "); 
	if (definition)
		fprintf(fout, "%s::", mname);
	fprintf(fout, "%s%s", prefix, fname);
}

static void
gengetter(struct nbuf_b2h *ctx, FILE *fout,
	nbuf_FieldDesc fld, const char *mname,
	bool definition)
{
	const char *fname = nbuf_FieldDesc_name(fld, NULL);
	nbuf_Kind kind = nbuf_FieldDesc_kind(fld);
	bool list = nbuf_FieldDesc_list(fld);
	uint32_t tag0 = nbuf_FieldDesc_tag0(fld);
	uint32_t tag1 = nbuf_FieldDesc_tag1(fld);
	const char *typ = typestr(ctx, kind, tag1);
	const char *o = "this";
	const char *comma = "";

	outfnhdr(fout, typ, mname, "", fname, definition);
	fprintf(fout, "(");
	if (list) {
		fprintf(fout, "size_t i");
		comma = ", ";
	}
	if (kind == nbuf_Kind_STR) {
		fprintf(fout, "%ssize_t *lenp%s",
			comma, definition ? "" : " = NULL");
		comma = ", ";
	}
	fprintf(fout, ") const%s\n",
		definition ? "\n{" : ";");
	if (!definition) {
		if (list)
			fprintf(fout, "\tinline size_t %s_size() const;\n",
				fname);
		return;
	}
	if (list) {
		fprintf(fout, "\tnbuf_obj t = nbuf_get_ptr(this, %u);\n",
			tag0);
		tag0 = 0;
		fprintf(fout, "\tt = nbuf_get_elem(&t, i);\n");
		o = "&t";
	}
	switch (kind) {
	case nbuf_Kind_BOOL:
		fprintf(fout, "\treturn nbuf_get_bit(%s, ", o);
		if (!list)
			fprintf(fout, "%u);\n", tag0);
		else
			fprintf(fout, "i%%8);\n");
		break;
	case nbuf_Kind_INT:
	case nbuf_Kind_UINT:
		fprintf(fout, "\treturn nbuf_get_int(%s, %u, %u);\n",
			o, tag0, tag1);
		break;
	case nbuf_Kind_ENUM:
		fprintf(fout, "\treturn (%s) nbuf_get_int(%s, %u, 2);\n",
			typ, o, tag0);
		break;
	case nbuf_Kind_FLOAT:
		fprintf(fout, "\treturn nbuf_get_f%d(%s, %u);\n",
			tag1 * 8, o, tag0);
		break;
	case nbuf_Kind_STR:
		fprintf(fout, "\treturn nbuf_get_str(%s, %u, lenp);\n",
			o, tag0);
		break;
	case nbuf_Kind_PTR:
		if (!list)
			fprintf(fout, "\treturn "
				"%s(nbuf_get_ptr(this, %u));\n", typ, tag0);
		else
			fprintf(fout, "\treturn %s(t);\n", typ);
		break;
	default:
		assert(0 && "bad kind");
	}
	fprintf(fout, "}\n");

	// restore tag0 in case it was zeroed
	tag0 = nbuf_FieldDesc_tag0(fld);
	if (list) { // size getter
		fprintf(fout, "\nsize_t\n");
		fprintf(fout, "%s::%s_size() const\n{\n", mname, fname);
		fprintf(fout, "\tnbuf_obj t = nbuf_get_ptr(this, %u);\n",
			tag0);
		fprintf(fout, "\treturn t.nelem;\n");
		fprintf(fout, "}\n");
	}
}

static void
gensetter(struct nbuf_b2h *ctx, FILE *fout,
	nbuf_FieldDesc fld, const char *mname,
	bool definition)
{
	const char *fname = nbuf_FieldDesc_name(fld, NULL);
	nbuf_Kind kind = nbuf_FieldDesc_kind(fld);
	bool list = nbuf_FieldDesc_list(fld);
	uint32_t tag0 = nbuf_FieldDesc_tag0(fld);
	uint32_t tag1 = nbuf_FieldDesc_tag1(fld);
	const char *typ = typestr(ctx, kind, tag1);
	const char *o = "this";

	if (kind == nbuf_Kind_PTR)
		return;  /* no setter */
	outfnhdr(fout, "void", mname, "set_", fname, definition);
	fprintf(fout, "(%s%s v%s)%s\n",
		list ? "size_t i, " : "", typ,
		(kind == nbuf_Kind_STR)
			?  definition
				? ", size_t len"
				: ", size_t len = -1"
			: "",
		definition ? "\n{" : ";"); 
	if (!definition)
		return;
	if (list) {
		fprintf(fout, "\tnbuf_obj t = nbuf_get_ptr(this, %u);\n",
			tag0);
		tag0 = 0;
		fprintf(fout, "\tt = nbuf_get_elem(&t, i);\n");
		o = "&t";
	}
	switch (kind) {
	case nbuf_Kind_BOOL:
		fprintf(fout, "\treturn nbuf_put_bit(%s, ", o);
		if (!list)
			fprintf(fout, "%u, v);\n", tag0);
		else
			fprintf(fout, "i%%8, v);\n");
		break;
	case nbuf_Kind_INT:
	case nbuf_Kind_UINT:
		fprintf(fout, "\treturn nbuf_put_int(%s, %u, %u, v);\n",
			o, tag0, tag1);
		break;
	case nbuf_Kind_ENUM:
		fprintf(fout, "\treturn nbuf_put_int(%s, %u, 2, "
			"(uint16_t) v);\n", o, tag0);
		break;
	case nbuf_Kind_FLOAT:
		fprintf(fout, "\treturn nbuf_put_f%d(%s, %u, v);\n",
			tag1 * 8, o, tag0);
		break;
	case nbuf_Kind_STR:
		fprintf(fout, "\treturn nbuf_put_str(%s, %u, v, len);\n",
			o, tag0);
		break;
	default:
		assert(0 && "bad kind");
	}
	fprintf(fout, "}\n");
}

static void
geniniter(struct nbuf_b2h *ctx, FILE *fout,
	nbuf_FieldDesc fld, const char *mname,
	bool definition)
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
	outfnhdr(fout, list ? "void" : typ,
		mname, "init_", fname, definition);
	fprintf(fout, "(%s)%s\n",
		list ? "size_t n" : "",
		definition ? "\n{" : ";");
	if (!definition)
		return;
	_nbuf_get_size(&o, ctx->schema, kind, tag1);
	fprintf(fout, "\tnbuf_obj oo = nbuf_create(buf, %s, %u, %u);\n",
		list ? "n" : "1", o.ssize, o.psize);
	fprintf(fout, "\tnbuf_put_ptr(this, %u, oo);\n", tag0);
	if (!list)
		fprintf(fout, "return %s(oo);", typ);
	fprintf(fout, "}\n");
}

static void
genhasfld(struct nbuf_b2h *ctx, FILE *fout,
	nbuf_FieldDesc fld, const char *mname,
	bool definition)
{
	const char *fname = nbuf_FieldDesc_name(fld, NULL);
	nbuf_Kind kind = nbuf_FieldDesc_kind(fld);
	bool list = nbuf_FieldDesc_list(fld);
	uint32_t tag0 = nbuf_FieldDesc_tag0(fld);

	if (kind != nbuf_Kind_PTR && !list)
		return;
	outfnhdr(fout, "bool", mname, "has_", fname, definition);
	fprintf(fout, "() const%s\n",
		definition ? "\n{" : ";");
	if (!definition)
		return;
	fprintf(fout, "\treturn nbuf_has_ptr(this, %u);\n", tag0);
	fprintf(fout, "}\n");
}

static void
genmsgs(struct nbuf_b2h *ctx, FILE *fout)
{
	size_t n = nbuf_Schema_msgTypes_size(ctx->schema);
	size_t i;

	/* 1st pass - class declarations */
	fprintf(fout, "\n");
	for (i = 0; i < n; i++) {
		nbuf_MsgType m = nbuf_Schema_msgTypes(ctx->schema, i);
		const char *mname = nbuf_MsgType_name(m, NULL);
		fprintf(fout, "struct %s;\n", mname);
	}

	/* 2nd pass - class definitions, method declarations */
	for (i = 0; i < n; i++) {
		nbuf_MsgType msg = nbuf_Schema_msgTypes(ctx->schema, i);
		const char *mname = nbuf_MsgType_name(msg, NULL);
		uint16_t ssize = nbuf_MsgType_ssize(msg);
		uint16_t psize = nbuf_MsgType_psize(msg);
		size_t j, m = nbuf_MsgType_fields_size(msg);

		fprintf(fout, "\nstruct %s : public nbuf_obj {\n", mname);
		/* ctor */
		fprintf(fout, "\texplicit %s(nbuf_obj o) : "
			"nbuf_obj(o) {}\n\n", mname);
		for (j = 0; j < m; j++) {
			nbuf_FieldDesc fld = nbuf_MsgType_fields(msg, j);
			gengetter(ctx, fout, fld, mname, false);
			gensetter(ctx, fout, fld, mname, false);
			geniniter(ctx, fout, fld, mname, false);
			genhasfld(ctx, fout, fld, mname, false);
		}
		fprintf(fout, "};\n");

		/* reader */
		fprintf(fout, "\nstatic inline %s\n", mname);
		fprintf(fout, "get_%s(nbuf_buffer *buf)\n", mname);
		fprintf(fout, "{\n\tnbuf_obj o = {buf};\n"
			"\tnbuf_obj_init(&o, 0);\n\treturn %s(o);\n}\n",
			mname);

		/* writer */
		fprintf(fout, "\nstatic inline %s\n", mname);
		fprintf(fout, "new_%s(nbuf_buffer *buf)\n", mname);
		fprintf(fout,
			"{\n\treturn %s(nbuf_create(buf, 1, %u, %u));\n}\n",
			mname, ssize, psize);
	}
	/* 3rd pass - method definitions */
	for (i = 0; i < n; i++) {
		nbuf_MsgType msg = nbuf_Schema_msgTypes(ctx->schema, i);
		const char *mname = nbuf_MsgType_name(msg, NULL);
		size_t j, m = nbuf_MsgType_fields_size(msg);
		for (j = 0; j < m; j++) {
			nbuf_FieldDesc fld = nbuf_MsgType_fields(msg, j);
			gengetter(ctx, fout, fld, mname, true);
			gensetter(ctx, fout, fld, mname, true);
			geniniter(ctx, fout, fld, mname, true);
			genhasfld(ctx, fout, fld, mname, true);
		}
	}
}

static void
outhdr(struct nbuf_b2h *ctx, FILE *fout, const char *srcname)
{
	fprintf(fout, "// Generated from %s.  DO NOT EDIT.\n\n", srcname);
	fprintf(fout, "#ifndef %s\n#define %s\n\n", ctx->guard, ctx->guard);
	fprintf(fout, "#include \"nbuf.h\"\n");
	fprintf(fout, "#include \"nbuf.nb.h\"\n");
	if (ctx->prefix[0] != '\0')
		fprintf(fout, "\nnamespace %s {\n", ctx->prefix);
}

static void
outftr(struct nbuf_b2h *ctx, FILE *fout)
{
	size_t n;
	size_t i;

	if (ctx->prefix[0] != '\0')
		fprintf(fout, "\n}  // namespace %s\n", ctx->prefix);
	n = nbuf_Schema_enumTypes_size(ctx->schema);
	if (n > 0)
		fprintf(fout, "\nextern nbuf_EnumType\n");
	for (i = 0; i < n; i++) {
		nbuf_EnumType e = nbuf_Schema_enumTypes(ctx->schema, i);
		const char *ename = nbuf_EnumType_name(e, NULL);
		fprintf(fout, "%s_refl_%s%c\n",
			ctx->prefix, ename, (i == n-1) ? ';' : ',');
	}
	n = nbuf_Schema_msgTypes_size(ctx->schema);
	if (n > 0)
		fprintf(fout, "\nextern nbuf_MsgType\n");
	for (i = 0; i < n; i++) {
		nbuf_MsgType m = nbuf_Schema_msgTypes(ctx->schema, i);
		const char *mname = nbuf_MsgType_name(m, NULL);
		fprintf(fout, "%s_refl_%s%c\n",
			ctx->prefix, mname, (i == n-1) ? ';' : ',');
	}
	fprintf(fout, "\nextern nbuf_Schema %s_refl_schema;\n", ctx->prefix);
	fprintf(fout, "\n#endif  // %s\n", ctx->guard);
}

static void
cleanup(struct nbuf_b2h *ctx)
{
	free(ctx->prefix);
	free(ctx->guard);
}

void
nbuf_b2hh(struct nbuf_buffer *buf, FILE *fout, const char *srcname)
{
	struct nbuf_b2h ctx[1];

	ctx->schema = nbuf_get_Schema(buf);
	makeprefix(ctx, srcname, "", "_HPP");
	outhdr(ctx, fout, srcname);
	genenums(ctx, fout);
	genmsgs(ctx, fout);
	outftr(ctx, fout);
	cleanup(ctx);
}
