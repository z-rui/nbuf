/* Generate header from binary schema. */

#include "nbuf.h"
#include "nbuf.nb.h"

#include <assert.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>

extern void load(const char *filename, void **msg, size_t *size);

nbuf_Schema schema;
char *prefix, *upper_prefix;
FILE *fout;

void
makeprefix()
{
	size_t i, len;
	const char *pkgName = nbuf_Schema_pkgName(&schema, &len);
	char ch;

	if (len == 0) {
		prefix = "";
		return;
	}
	/* pkg.name => pkg_name_, PKG_NAME_NB_H */
	prefix = malloc(len + 2);
	upper_prefix = malloc(len + 6);
	for (i = 0; i < len; i++) {
		ch = pkgName[i];
		if (!isalnum(ch))
			ch = '_';
		prefix[i] = ch;
		upper_prefix[i] = toupper(ch);
	}
	strcpy(prefix + len, "_");
	strcpy(upper_prefix + len, "_NB_H");
}

void genenums()
{
	size_t n = nbuf_Schema_enumTypes_size(&schema);
	size_t i;

	for (i = 0; i < n; i++) {
		nbuf_EnumType e = nbuf_Schema_enumTypes(&schema, i);
		const char *ename = nbuf_EnumType_name(&e, NULL);
		size_t m = nbuf_EnumType_values_size(&e);
		size_t j;

		fprintf(fout, "\ntypedef enum {\n");
		for (j = 0; j < m; j++) {
			nbuf_EnumDesc d = nbuf_EnumType_values(&e, j);
			const char *name = nbuf_EnumDesc_name(&d, NULL);
			uint16_t value = nbuf_EnumDesc_value(&d);
			fprintf(fout, "\t%s%s_%s = %d,\n",
				prefix, ename, name, value);
		}
		fprintf(fout, "} %s%s;\n", prefix, ename);
	}
}

const char *
typestr(nbuf_Kind kind, uint32_t tag1)
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
		sprintf(buf, "float%d_t", tag1*8);
		return buf;
	case nbuf_Kind_STR:
		return "const char *";
	case nbuf_Kind_ENUM:
		enm = nbuf_Schema_enumTypes(&schema, tag1);
		return nbuf_EnumType_name(&enm, NULL);
	case nbuf_Kind_PTR:
		msg = nbuf_Schema_msgTypes(&schema, tag1);
		return nbuf_MsgType_name(&msg, NULL);
	default:
		assert(0 && "unknown kind");
		break;
	}
}

static const char *
maybeprefix(nbuf_Kind kind)
{
	if (kind == nbuf_Kind_ENUM || kind == nbuf_Kind_PTR)
		return prefix;
	return "";
}

void gengetter(nbuf_FieldDesc fld, const char *mname)
{
	const char *fname = nbuf_FieldDesc_name(&fld, NULL);
	nbuf_Kind kind = nbuf_FieldDesc_kind(&fld);
	bool list = nbuf_FieldDesc_list(&fld);
	uint32_t tag0 = nbuf_FieldDesc_tag0(&fld);
	uint32_t tag1 = nbuf_FieldDesc_tag1(&fld);
	const char *typ = typestr(kind, tag1);
	const char *o = "&o->o";

	fprintf(fout, "\nstatic inline %s%s\n", maybeprefix(kind), typ); 
	fprintf(fout, "%s%s_%s(const %s%s *o%s%s)\n{\n",
		prefix, mname, fname, prefix, mname,
		list ? ", size_t i" : "",
		(kind == nbuf_Kind_STR) ? ", size_t *lenp" : ""); 
	if (kind == nbuf_Kind_PTR)
		fprintf(fout, "\t%s%s oo;\n", prefix, typ);
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
			fprintf(fout, "\too.o = nbuf_get_ptr(o, %u);\n", tag0);
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
			prefix, mname, fname, prefix, mname);
		fprintf(fout,
			"\tstruct nbuf_obj t = nbuf_get_ptr(&o->o, %u);\n",
			tag0);
		fprintf(fout, "\treturn t.nelem;\n");
		fprintf(fout, "}\n");
	}
}

void
gensetter(nbuf_FieldDesc fld, const char *mname)
{
	const char *fname = nbuf_FieldDesc_name(&fld, NULL);
	nbuf_Kind kind = nbuf_FieldDesc_kind(&fld);
	bool list = nbuf_FieldDesc_list(&fld);
	uint32_t tag0 = nbuf_FieldDesc_tag0(&fld);
	uint32_t tag1 = nbuf_FieldDesc_tag1(&fld);
	const char *typ = typestr(kind, tag1);
	const char *o = "&o->o";

	if (kind == nbuf_Kind_PTR)
		return;  /* no setter */
	fprintf(fout, "\nstatic inline void\n");
	fprintf(fout, "%s%s_set_%s(const %s%s *o%s, %s%s v%s)\n{\n",
		prefix, mname, fname, prefix, mname,
		list ? ", size_t i" : "",
		maybeprefix(kind), typ,
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

void
getsizeinfo(unsigned msgtype_id, uint16_t *ssize, uint16_t *psize)
{
	nbuf_MsgType msg = nbuf_Schema_msgTypes(&schema, msgtype_id);
	*ssize = nbuf_MsgType_ssize(&msg);
	*psize = nbuf_MsgType_psize(&msg);
}

void
geniniter(nbuf_FieldDesc fld, const char *mname)
{
	const char *fname = nbuf_FieldDesc_name(&fld, NULL);
	nbuf_Kind kind = nbuf_FieldDesc_kind(&fld);
	bool list = nbuf_FieldDesc_list(&fld);
	uint32_t tag0 = nbuf_FieldDesc_tag0(&fld);
	uint32_t tag1 = nbuf_FieldDesc_tag1(&fld);
	uint16_t ssize = 0, psize = 0;

	if (kind != nbuf_Kind_PTR && !list)
		return;
	fprintf(fout, "\nstatic inline void\n");
	fprintf(fout, "%s%s_init_%s(const %s%s *o%s)\n{\n",
		prefix, mname, fname, prefix, mname,
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
		getsizeinfo(tag1, &ssize, &psize);
		break;
	default:
		assert(0 && "bad kind");
	}
	fprintf(fout, "\tstruct nbuf_obj oo = "
		"nbuf_create(o->o.buf, %s, %u, %u);\n",
		list ? "n" : "1", ssize, psize);
	fprintf(fout, "\tnbuf_put_ptr(&o->o, %u, oo);\n", tag0);
	fprintf(fout, "}\n");
}

void
genhasfld(nbuf_FieldDesc fld, const char *mname)
{
	const char *fname = nbuf_FieldDesc_name(&fld, NULL);
	nbuf_Kind kind = nbuf_FieldDesc_kind(&fld);
	bool list = nbuf_FieldDesc_list(&fld);
	uint32_t tag0 = nbuf_FieldDesc_tag0(&fld);

	if (kind != nbuf_Kind_PTR && !list)
		return;
	fprintf(fout, "\nstatic inline bool\n");
	fprintf(fout, "%s%s_has_%s(const %s%s *o)\n{\n",
		prefix, mname, fname, prefix, mname);
	fprintf(fout, "\treturn nbuf_has_ptr(&o->o, %u);\n", tag0);
	fprintf(fout, "}\n");
}

void genmsgs()
{
	size_t n = nbuf_Schema_msgTypes_size(&schema);
	size_t i;

	/* 1st pass - declare all types */
	for (i = 0; i < n; i++) {
		nbuf_MsgType m = nbuf_Schema_msgTypes(&schema, i);
		const char *mname = nbuf_MsgType_name(&m, NULL);
		uint16_t ssize = nbuf_MsgType_ssize(&m);
		uint16_t psize = nbuf_MsgType_psize(&m);

		/* struct */
		fprintf(fout, "\ntypedef struct {\n"
			"\tstruct nbuf_obj o;\n} %s%s;\n",
			prefix, mname);

		/* reader */
		fprintf(fout, "\nstatic inline %s%s\n", prefix, mname);
		fprintf(fout, "%sget_%s(struct nbuf_buffer *buf)\n",
			prefix, mname);
		fprintf(fout, "{\n\t%s%s o = {{buf}};\n"
			"\tnbuf_obj_init(&o.o, 0);\n\treturn o;\n}\n",
			prefix, mname);

		/* writer */
		fprintf(fout, "\nstatic inline %s%s\n", prefix, mname);
		fprintf(fout, "%snew_%s(struct nbuf_buffer *buf)\n",
			prefix, mname);
		fprintf(fout, "{\n\t%s%s o;\n"
			"\to.o = nbuf_create(buf, 1, %u, %u);\n\treturn o;\n}\n",
			prefix, mname, ssize, psize);
	}
	/* 2st pass - declare all accessors */
	for (i = 0; i < n; i++) {
		nbuf_MsgType msg = nbuf_Schema_msgTypes(&schema, i);
		const char *mname = nbuf_MsgType_name(&msg, NULL);
		size_t j, m = nbuf_MsgType_fields_size(&msg);
		for (j = 0; j < m; j++) {
			nbuf_FieldDesc fld = nbuf_MsgType_fields(&msg, j);
			gengetter(fld, mname);
			gensetter(fld, mname);
			geniniter(fld, mname);
			genhasfld(fld, mname);
		}
	}
}

void
outhdr(const char *infile)
{
	fprintf(fout, "/* Generated from %s.  DO NOT EDIT. */\n\n", infile);
	fprintf(fout, "#ifndef %s\n#define %s\n\n", upper_prefix, upper_prefix);
	fprintf(fout, "#include \"nbuf.h\"\n");
}

void
outftr()
{
	fprintf(fout, "\n#endif  /* %s */\n", upper_prefix);
}

void
cleanup()
{
	free(prefix);
	free(upper_prefix);
}

int
main(int argc, char *argv[])
{
	struct nbuf_buffer buf = {NULL};
	void *base;
	size_t len;

	load(argv[1], &base, &len);
	nbuf_init_read(&buf, base, len);
	schema = nbuf_get_Schema(&buf);
	makeprefix();

	fout = stdout;
	outhdr(argv[1]);
	genenums();
	genmsgs();
	outftr();
	cleanup();

	return 0;
}
