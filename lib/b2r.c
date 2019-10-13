/* Generate header from binary schema. */

#include "nbuf.nb.h"
#include "libnbuf.h"

#include <assert.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>

struct nbuf_b2r {
	nbuf_Schema schema;
	char *prefix;
};

static void
outbuf(struct nbuf_b2r *b2r, FILE *fout, struct nbuf_buffer *buf)
{
	size_t i;

	fprintf(fout, "\nstatic const unsigned char buf_data[] = {");
	/* 16 numbers per line => 80 characters */
	for (i = 0; i < buf->len; i++) {
		fprintf(fout, "%s%3u,",
			(i % 16 == 0) ? "\n" : " ",
			buf->base[i] & 0xff);
	}
	fprintf(fout, "\n};\n");
	fprintf(fout, "\nstatic struct nbuf_buffer buf = "
		"{(char *) buf_data, sizeof buf_data, 0};\n");
}

static void
genenums(struct nbuf_b2r *b2r, FILE *fout)
{
	size_t n = nbuf_Schema_enumTypes_size(&b2r->schema);
	size_t i;

	for (i = 0; i < n; i++) {
		nbuf_EnumType e = nbuf_Schema_enumTypes(&b2r->schema, i);
		const char *ename = nbuf_EnumType_name(&e, NULL);

		fprintf(fout, "\nnbuf_EnumType %sschema_%s = ",
			b2r->prefix, ename);
		fprintf(fout, "{{&buf, 1, %u, %u}};\n", e.o.ssize, e.o.psize);
	}
}

static void
genmsgs(struct nbuf_b2r *b2r, FILE *fout)
{
	size_t n = nbuf_Schema_msgTypes_size(&b2r->schema);
	size_t i;

	/* 1st pass - declare all types */
	for (i = 0; i < n; i++) {
		nbuf_MsgType m = nbuf_Schema_msgTypes(&b2r->schema, i);
		const char *mname = nbuf_MsgType_name(&m, NULL);

		fprintf(fout, "\nnbuf_MsgType %sschema_%s = ",
			b2r->prefix, mname);
		fprintf(fout, "{{&buf, %lu, 1, %u, %u}};\n",
			(unsigned long) m.o.base, m.o.ssize, m.o.psize);
	}
}

static void
outhdr(struct nbuf_b2r *b2r, FILE *fout, const char *srcname)
{
	size_t i, len;
	const char *pkgName = nbuf_Schema_pkgName(&b2r->schema, &len);
	char ch;

	if (len == 0) {
		b2r->prefix = strdup("");
	} else {
		/* pkg.name => pkg_name_ */
		b2r->prefix = malloc(len + 2);
		for (i = 0; i < len; i++) {
			ch = pkgName[i];
			if (!isalnum(ch))
				ch = '_';
			b2r->prefix[i] = ch;
		}
		strcpy(b2r->prefix + len, "_");
	}

	fprintf(fout, "/* Generated from %s.  DO NOT EDIT. */\n\n",
		srcname);
	fprintf(fout, "#include \"libnbuf.h\"\n");
}

static void
outftr(struct nbuf_b2r *b2r, FILE *fout)
{
	struct nbuf_obj *o = &b2r->schema.o;
	fprintf(fout, "\nnbuf_Schema %sschema = ", b2r->prefix);
	fprintf(fout, "{{&buf, %lu, 1, %u, %u}};\n",
		(unsigned long) o->base, o->ssize, o->psize);
}

static void
cleanup(struct nbuf_b2r *b2r)
{
	free(b2r->prefix);
}

void
nbuf_b2r(struct nbuf_buffer *buf, FILE *fout, const char *srcname)
{
	struct nbuf_b2r b2r;

	b2r.schema = nbuf_get_Schema(buf);
	outhdr(&b2r, fout, srcname);
	outbuf(&b2r, fout, buf);
	genenums(&b2r, fout);
	genmsgs(&b2r, fout);
	outftr(&b2r, fout);
	cleanup(&b2r);
}
