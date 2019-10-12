/* Generate header from binary schema. */

#include "nbuf.nb.h"
#include "libnbuf.h"

#include <assert.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>

struct nbuf_b2h {
	nbuf_Schema schema;
	char *prefix, *upper_prefix;
};

static void
genenums(struct nbuf_b2h *b2h, FILE *fout)
{
	size_t n = nbuf_Schema_enumTypes_size(&b2h->schema);
	size_t i;

	for (i = 0; i < n; i++) {
		nbuf_EnumType e = nbuf_Schema_enumTypes(&b2h->schema, i);
		const char *ename = nbuf_EnumType_name(&e, NULL);
		size_t m = nbuf_EnumType_values_size(&e);
		size_t j;

		fprintf(fout, "\ntypedef enum {\n");
		for (j = 0; j < m; j++) {
			nbuf_EnumDesc d = nbuf_EnumType_values(&e, j);
			const char *name = nbuf_EnumDesc_name(&d, NULL);
			uint16_t value = nbuf_EnumDesc_value(&d);
			fprintf(fout, "\t%s%s_%s = %d,\n",
				b2h->prefix, ename, name, value);
		}
		fprintf(fout, "} %s%s;\n", b2h->prefix, ename);
	}
}

static const char *
typestr(nbuf_Schema *schema, nbuf_Kind kind, uint32_t tag1)
{
	static char buf[20];
	nbuf_EnumType enm;
	nbuf_MsgType msg;

	switch (kind) {
	case nbuf_Kind_BOOL:
		return "bool";
	case nbuf_Kind_INT:
		sprintf(buf, "int%d_t", tag1*8);
		return buf;
	case nbuf_Kind_UINT:
		sprintf(buf, "uint%d_t", tag1*8);
		return buf;
	case nbuf_Kind_FLOAT:
		return (tag1 == 4) ? "float" : "double";
	case nbuf_Kind_STR:
		return "const char *";
	case nbuf_Kind_ENUM:
		enm = nbuf_Schema_enumTypes(schema, tag1);
		return nbuf_EnumType_name(&enm, NULL);
	case nbuf_Kind_PTR:
		msg = nbuf_Schema_msgTypes(schema, tag1);
		return nbuf_MsgType_name(&msg, NULL);
	default:
		assert(0 && "unknown kind");
		break;
	}
	return NULL;
}

static const char *
maybeprefix(struct nbuf_b2h *b2h, nbuf_Kind kind)
{
	if (kind == nbuf_Kind_ENUM || kind == nbuf_Kind_PTR)
		return b2h->prefix;
	return "";
}

static void
gengetter(struct nbuf_b2h *b2h, FILE *fout,
	nbuf_FieldDesc fld, const char *mname)
{
	const char *fname = nbuf_FieldDesc_name(&fld, NULL);
	nbuf_Kind kind = nbuf_FieldDesc_kind(&fld);
	bool list = nbuf_FieldDesc_list(&fld);
	uint32_t tag0 = nbuf_FieldDesc_tag0(&fld);
	uint32_t tag1 = nbuf_FieldDesc_tag1(&fld);
	const char *typ = typestr(&b2h->schema, kind, tag1);
	const char *o = "&o->o";

	fprintf(fout, "\nstatic inline %s%s\n", maybeprefix(b2h, kind), typ); 
	fprintf(fout, "%s%s_%s(const %s%s *o%s%s)\n{\n",
		b2h->prefix, mname, fname, b2h->prefix, mname,
		list ? ", size_t i" : "",
		(kind == nbuf_Kind_STR) ? ", size_t *lenp" : ""); 
	if (kind == nbuf_Kind_PTR)
		fprintf(fout, "\t%s%s oo;\n", b2h->prefix, typ);
	if (list) {
		fprintf(fout,
			"\tstruct nbuf_obj t = nbuf_get_ptr(&o->o, %u);\n",
			tag0);
		tag0 = 0;
		fprintf(fout, "\tt = nbuf_get_elem(&t, %s);\n",
			(kind == nbuf_Kind_BOOL) ? "i/8" : "i");
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
		fprintf(fout, "\treturn nbuf_get_int(%s, %u, 2);\n",
			o, tag0);
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
		if (!list) {
			fprintf(fout, "\too.o = nbuf_get_ptr(&o->o, %u);\n",
				tag0);
		} else {
			fprintf(fout, "\too.o = t;\n");
		}
		fprintf(fout, "\treturn oo;\n");
		break;
	default:
		assert(0 && "bad kind");
	}
	fprintf(fout, "}\n");

	// restore tag0 in case it was zeroed
	tag0 = nbuf_FieldDesc_tag0(&fld);
	if (list) { // size getter
		fprintf(fout, "\nstatic inline size_t\n");
		fprintf(fout, "%s%s_%s_size(const %s%s *o)\n{\n",
			b2h->prefix, mname, fname, b2h->prefix, mname);
		fprintf(fout,
			"\tstruct nbuf_obj t = nbuf_get_ptr(&o->o, %u);\n",
			tag0);
		fprintf(fout, "\treturn t.nelem;\n");
		fprintf(fout, "}\n");
	}
}

static void
gensetter(struct nbuf_b2h *b2h, FILE *fout,
	nbuf_FieldDesc fld, const char *mname)
{
	const char *fname = nbuf_FieldDesc_name(&fld, NULL);
	nbuf_Kind kind = nbuf_FieldDesc_kind(&fld);
	bool list = nbuf_FieldDesc_list(&fld);
	uint32_t tag0 = nbuf_FieldDesc_tag0(&fld);
	uint32_t tag1 = nbuf_FieldDesc_tag1(&fld);
	const char *typ = typestr(&b2h->schema, kind, tag1);
	const char *o = "&o->o";

	if (kind == nbuf_Kind_PTR)
		return;  /* no setter */
	fprintf(fout, "\nstatic inline void\n");
	fprintf(fout, "%s%s_set_%s(const %s%s *o%s, %s%s v%s)\n{\n",
		b2h->prefix, mname, fname, b2h->prefix, mname,
		list ? ", size_t i" : "",
		maybeprefix(b2h, kind), typ,
		(kind == nbuf_Kind_STR) ? ", size_t len" : ""); 
	if (list) {
		fprintf(fout,
			"\tstruct nbuf_obj t = nbuf_get_ptr(&o->o, %u);\n",
			tag0);
		tag0 = 0;
		fprintf(fout, "\tt = nbuf_get_elem(&t, %s);\n",
			(kind == nbuf_Kind_BOOL) ? "i/8" : "i");
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
		fprintf(fout, "\treturn nbuf_put_int(%s, %u, 2, v);\n",
			o, tag0);
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
getsizeinfo(nbuf_Schema *schema,
	unsigned msgtype_id,
	uint16_t *ssize,
	uint16_t *psize)
{
	nbuf_MsgType msg = nbuf_Schema_msgTypes(schema, msgtype_id);
	*ssize = nbuf_MsgType_ssize(&msg);
	*psize = nbuf_MsgType_psize(&msg);
}

static void
geniniter(struct nbuf_b2h *b2h, FILE *fout,
	nbuf_FieldDesc fld, const char *mname)
{
	const char *fname = nbuf_FieldDesc_name(&fld, NULL);
	nbuf_Kind kind = nbuf_FieldDesc_kind(&fld);
	bool list = nbuf_FieldDesc_list(&fld);
	uint32_t tag0 = nbuf_FieldDesc_tag0(&fld);
	uint32_t tag1 = nbuf_FieldDesc_tag1(&fld);
	uint16_t ssize = 0, psize = 0;

	if (kind != nbuf_Kind_PTR && !list)
		return;
	fprintf(fout, "\nstatic inline ");
	if (list)
		fprintf(fout, "void");
	else
		fprintf(fout, "%s%s", b2h->prefix,
			typestr(&b2h->schema, kind, tag1));
	fprintf(fout, "\n%s%s_init_%s(const %s%s *o%s)\n{\n",
		b2h->prefix, mname, fname, b2h->prefix, mname,
		list ? ", size_t n" : "");
	switch (kind) {
	case nbuf_Kind_BOOL:
		ssize = 1;
		break;
	case nbuf_Kind_INT:
	case nbuf_Kind_UINT:
	case nbuf_Kind_FLOAT:
		ssize = tag1;
		break;
	case nbuf_Kind_ENUM:
		ssize = 2;
		break;
	case nbuf_Kind_STR:
		psize = 1;
		break;
	case nbuf_Kind_PTR:
		getsizeinfo(&b2h->schema, tag1, &ssize, &psize);
		break;
	default:
		assert(0 && "bad kind");
	}
	fprintf(fout, "\tstruct nbuf_obj oo = "
		"nbuf_create(o->o.buf, %s, %u, %u);\n",
		list ? "n" : "1", ssize, psize);
	fprintf(fout, "\tnbuf_put_ptr(&o->o, %u, oo);\n", tag0);
	if (!list) {
		fprintf(fout, "\t%s%s t = {oo};\n\treturn t;",
			b2h->prefix, typestr(&b2h->schema, kind, tag1));
	}
	fprintf(fout, "}\n");
}

static void
genhasfld(struct nbuf_b2h *b2h, FILE *fout,
	nbuf_FieldDesc fld, const char *mname)
{
	const char *fname = nbuf_FieldDesc_name(&fld, NULL);
	nbuf_Kind kind = nbuf_FieldDesc_kind(&fld);
	bool list = nbuf_FieldDesc_list(&fld);
	uint32_t tag0 = nbuf_FieldDesc_tag0(&fld);

	if (kind != nbuf_Kind_PTR && !list)
		return;
	fprintf(fout, "\nstatic inline bool\n");
	fprintf(fout, "%s%s_has_%s(const %s%s *o)\n{\n",
		b2h->prefix, mname, fname, b2h->prefix, mname);
	fprintf(fout, "\treturn nbuf_has_ptr(&o->o, %u);\n", tag0);
	fprintf(fout, "}\n");
}

static void
genmsgs(struct nbuf_b2h *b2h, FILE *fout)
{
	size_t n = nbuf_Schema_msgTypes_size(&b2h->schema);
	size_t i;

	/* 1st pass - declare all types */
	for (i = 0; i < n; i++) {
		nbuf_MsgType m = nbuf_Schema_msgTypes(&b2h->schema, i);
		const char *mname = nbuf_MsgType_name(&m, NULL);
		uint16_t ssize = nbuf_MsgType_ssize(&m);
		uint16_t psize = nbuf_MsgType_psize(&m);

		/* struct */
		fprintf(fout, "\ntypedef struct {\n"
			"\tstruct nbuf_obj o;\n} %s%s;\n",
			b2h->prefix, mname);

		/* reader */
		fprintf(fout, "\nstatic inline %s%s\n", b2h->prefix, mname);
		fprintf(fout, "%sget_%s(struct nbuf_buffer *buf)\n",
			b2h->prefix, mname);
		fprintf(fout, "{\n\t%s%s o = {{buf}};\n"
			"\tnbuf_obj_init(&o.o, 0);\n\treturn o;\n}\n",
			b2h->prefix, mname);

		/* writer */
		fprintf(fout, "\nstatic inline %s%s\n", b2h->prefix, mname);
		fprintf(fout, "%snew_%s(struct nbuf_buffer *buf)\n",
			b2h->prefix, mname);
		fprintf(fout, "{\n\t%s%s o;\n"
			"\to.o = nbuf_create(buf, 1, %u, %u);\n\treturn o;\n}\n",
			b2h->prefix, mname, ssize, psize);
	}
	/* 2st pass - declare all accessors */
	for (i = 0; i < n; i++) {
		nbuf_MsgType msg = nbuf_Schema_msgTypes(&b2h->schema, i);
		const char *mname = nbuf_MsgType_name(&msg, NULL);
		size_t j, m = nbuf_MsgType_fields_size(&msg);
		for (j = 0; j < m; j++) {
			nbuf_FieldDesc fld = nbuf_MsgType_fields(&msg, j);
			gengetter(b2h, fout, fld, mname);
			gensetter(b2h, fout, fld, mname);
			geniniter(b2h, fout, fld, mname);
			genhasfld(b2h, fout, fld, mname);
		}
	}
}

static void
outhdr(struct nbuf_b2h *b2h, FILE *fout, const char *srcname)
{
	size_t i, len;
	const char *pkgName = nbuf_Schema_pkgName(&b2h->schema, &len);
	char ch;

	if (len == 0) {
		b2h->prefix = strdup("");
		len = strlen(srcname);
		/* file.nb => FILE_NB_H */
		b2h->upper_prefix = malloc(len + 3);
		for (i = 0; i < len; i++) {
			ch = srcname[i];
			if (!isalnum(ch))
				ch = '_';
			b2h->upper_prefix[i] = toupper(ch);
		}
		strcpy(b2h->upper_prefix + len, "_H");
		return;
	}

	/* pkg.name => pkg_name_, PKG_NAME_NB_H */
	b2h->prefix = malloc(len + 2);
	b2h->upper_prefix = malloc(len + 6);
	for (i = 0; i < len; i++) {
		ch = pkgName[i];
		if (!isalnum(ch))
			ch = '_';
		b2h->prefix[i] = ch;
		b2h->upper_prefix[i] = toupper(ch);
	}
	strcpy(b2h->prefix + len, "_");
	strcpy(b2h->upper_prefix + len, "_NB_H");

	fprintf(fout, "/* Generated from %s.  DO NOT EDIT. */\n\n",
		srcname);
	fprintf(fout, "#ifndef %s\n#define %s\n\n",
		b2h->upper_prefix, b2h->upper_prefix);
	fprintf(fout, "#include \"nbuf.h\"\n");
}

static void
outftr(struct nbuf_b2h *b2h, FILE *fout)
{
	fprintf(fout, "\n#endif  /* %s */\n", b2h->upper_prefix);
}

static void
cleanup(struct nbuf_b2h *b2h)
{
	free(b2h->prefix);
	free(b2h->upper_prefix);
}

void
nbuf_b2h(struct nbuf_buffer *buf, FILE *fout, const char *srcname)
{
	struct nbuf_b2h b2h;

	b2h.schema = nbuf_get_Schema(buf);
	outhdr(&b2h, fout, srcname);
	genenums(&b2h, fout);
	genmsgs(&b2h, fout);
	outftr(&b2h, fout);
	cleanup(&b2h);
}
