#include "nbuf.h"
#include "nbuf.nb.h"
#include "t2b.h"

#include <stdio.h>
#include <stdlib.h>

struct schemaDesc schema;

struct lookup_entry {
	const char *name;
	nbuf_Kind kind;
	unsigned size;
	int id;
	void *data;
};

static int
lookup_cmp(const void *a, const void *b)
{
	return strcmp(
		((struct lookup_entry *) a)->name,
		((struct lookup_entry *) b)->name);
}

static struct lookup_entry *
lookup(struct lookup_entry *base, size_t n, const char *key)
{
	struct lookup_entry dummy = {key};
	return bsearch(&dummy, base, n, sizeof *base, lookup_cmp);
}

#define MAX_TYPES 8192

static size_t enumnames, msgnames;
static size_t types_size = 12;
static struct lookup_entry types[MAX_TYPES] = {
	{"bool", nbuf_Kind_BOOL, 0},
	{"int8", nbuf_Kind_INT, 1},
	{"int16", nbuf_Kind_INT, 2},
	{"int32", nbuf_Kind_INT, 4},
	{"int64", nbuf_Kind_INT, 8},
	{"uint8", nbuf_Kind_UINT, 1},
	{"uint16", nbuf_Kind_UINT, 2},
	{"uint32", nbuf_Kind_UINT, 4},
	{"uint64", nbuf_Kind_UINT, 8},
	{"float", nbuf_Kind_FLOAT, 4},
	{"double", nbuf_Kind_FLOAT, 8},
	{"string", nbuf_Kind_STR, 1},
};

#define die(...) do { \
	fprintf(stderr, __VA_ARGS__); \
	exit(1); \
} while (0)
#define warn(...) do { \
	fprintf(stderr, __VA_ARGS__); \
} while (0)

#ifndef NDEBUG
#define debug(...) do { \
	fprintf(stderr, __VA_ARGS__); \
} while (0)
#else
#define debug(...) do {} while (0)
#endif

static void
makelookup()
{
	struct enumType *e;
	struct msgType *m;
	int id;

	for (id = 0, e = schema.enums; e; id++, e = e->next) {
		struct lookup_entry *ent;
		if (types_size == MAX_TYPES)
			goto out_of_memory;
		ent = &types[types_size++];
		ent->name = e->name;
		ent->kind = nbuf_Kind_ENUM;
		ent->size = 2;
		ent->id = id;
		ent->data = e;
	}
	enumnames = id;
	for (id = 0, m = schema.msgs; m; id++, m = m->next) {
		struct lookup_entry *ent;
		if (types_size == MAX_TYPES)
			goto out_of_memory;
		ent = &types[types_size++];
		ent->name = m->name;
		ent->kind = nbuf_Kind_PTR;
		ent->size = NBUF_WORD_SZ;
		ent->id = id;
		ent->data = m;
	}
	msgnames = id;
	qsort(types, types_size, sizeof *types, lookup_cmp);
	for (id = 1; id < types_size; id++) {
		if (strcmp(types[id-1].name, types[id].name) == 0)
			die("%s redefined\n", types[id].name);
	}
	return;
out_of_memory:
	die("too many types\n");
}

static nbuf_Schema Schema;

static void
makeschema(struct nbuf_buffer *buffer)
{
	Schema = nbuf_new_Schema(buffer);
	if (schema.pkgName != NULL)
		nbuf_Schema_set_pkgName(&Schema, schema.pkgName, -1);
	if (enumnames > 0)
		nbuf_Schema_init_enumTypes(&Schema, enumnames);
	if (msgnames > 0)
		nbuf_Schema_init_msgTypes(&Schema, msgnames);
}

static void
outenums()
{
	struct enumType *e;
	int id;

	for (id = 0, e = schema.enums; e; id++, e = e->next) {
		struct enumDesc *d;
		size_t n;
		nbuf_EnumType en = nbuf_Schema_enumTypes(&Schema, id);
		nbuf_EnumType_set_name(&en, e->name, -1);
		for (n = 0, d = e->vals; d; d = d->next)
			n++;
		nbuf_EnumType_init_values(&en, n);
		for (n = 0, d = e->vals; d; n++, d = d->next) {
			nbuf_EnumDesc de = nbuf_EnumType_values(&en, n);
			nbuf_EnumDesc_set_name(&de, d->name, -1);
			nbuf_EnumDesc_set_value(&de, d->val);
		}
	}
}

struct padding_space {
	uint32_t offset;
	uint32_t size;
	struct padding_space *next;
};

static struct padding_space *pad_start, **pad_link;
static int pad_bit_offs;

static void
clear_padding_spaces()
{
	struct padding_space *p, *next;

	for (p = pad_start; p; p = next) {
		next = p->next;
		free(p);
	}
	pad_start = NULL;
	pad_link = &pad_start;
	pad_bit_offs = 0;
}

static size_t
find_padding_space(size_t size)
{
	bool pack_bool = false;
	struct padding_space *p, **link;
	size_t offset, padding;

	if (size == 0) {
		if (pad_bit_offs % 8 != 0) {
			return pad_bit_offs++;
		}
		pack_bool = true;
		size = 1;  /* try to find a byte */
	}
	for (link = &pad_start; (p = *link); link = &p->next) {
		offset = NBUF_ROUNDUP(p->offset, size);
		padding = offset - p->offset;
		if (p->size - padding >= size)
			break;
	}
	if (p == NULL)
		return -1;  /* no usable padding space */
	if (padding) {
		size_t remain = p->size - padding - size;
		p->size = padding;
		if (remain) {
			struct padding_space *q = malloc(sizeof *q);
			q->offset = p->offset + padding + size;
			q->size = remain;
			q->next = p->next;
			p->next = q;
		}
	} else if (p->size > size) {
		p->size -= size;
	} else {
		*link = p->next;
		free(p);
	}

	if (pack_bool) {
		pad_bit_offs = 8 * offset;
		return pad_bit_offs++;
	}
	return offset;
}

static void
add_padding_space(size_t offset, size_t size)
{
	struct padding_space *q = malloc(sizeof *q);
	q->offset = offset;
	q->size = size;
	q->next = NULL;
	*pad_link = q;
	pad_link = &q->next;
}

static size_t
calculate_offset(uint16_t *ssize, size_t size)
{
	size_t offset = find_padding_space(size);
	size_t padding = 0;

	if (offset == -1) {
		if (size == 0) {  /* bool */
			pad_bit_offs = 8 * *ssize;
			offset = pad_bit_offs++;
			(*ssize)++;
		} else {
			offset = NBUF_ROUNDUP(*ssize, size);
			padding = offset - *ssize;
			if (padding > 0)
				add_padding_space(*ssize, padding);
			*ssize += padding + size;
		}
	}
	return offset;
}

static void
outmsgs()
{
	struct msgType *m;
	int id;

	clear_padding_spaces();
	for (id = 0, m = schema.msgs; m; id++, m = m->next) {
		struct fldDesc *d;
		size_t n;  /* number of fields */
		uint16_t ssize = 0, psize = 0;
		nbuf_MsgType ms = nbuf_Schema_msgTypes(&Schema, id);

		nbuf_MsgType_set_name(&ms, m->name, -1);
		for (n = 0, d = m->flds; d; d = d->next)
			n++;
		nbuf_MsgType_init_fields(&ms, n);
		for (n = 0, d = m->flds; d; n++, d = d->next) {
			nbuf_FieldDesc de = nbuf_MsgType_fields(&ms, n);
			struct lookup_entry *ent;
			size_t offset;

			debug("%s.%s\n", m->name, d->name);
			ent = lookup(types, types_size, d->tname);
			if (ent == NULL)
				die("BUG: cannot find type \"%s\"\n",
					d->tname);
			nbuf_FieldDesc_set_name(&de, d->name, -1);
			nbuf_FieldDesc_set_list(&de, d->list);
			nbuf_FieldDesc_set_kind(&de, ent->kind);
			switch (ent->kind) {
			case nbuf_Kind_ENUM:
			case nbuf_Kind_BOOL:
			case nbuf_Kind_INT:
			case nbuf_Kind_UINT:
			case nbuf_Kind_FLOAT:
				if (!d->list) {
					offset = calculate_offset(&ssize, ent->size);
					nbuf_FieldDesc_set_tag0(&de, offset);
				} else {
			case nbuf_Kind_PTR:
			case nbuf_Kind_STR:
					nbuf_FieldDesc_set_tag0(&de, psize++);
				}
				break;
			}
			switch (ent->kind) {
			case nbuf_Kind_ENUM:
			case nbuf_Kind_PTR:
				nbuf_FieldDesc_set_tag1(&de, ent->id);
				break;
			case nbuf_Kind_BOOL:
			case nbuf_Kind_INT:
			case nbuf_Kind_UINT:
			case nbuf_Kind_FLOAT:
			case nbuf_Kind_STR:
				nbuf_FieldDesc_set_tag1(&de, ent->size);
				break;
			}
		}
		if (psize > 0)
			ssize = NBUF_ROUNDUP(ssize, NBUF_WORD_SZ);
		nbuf_MsgType_set_ssize(&ms, ssize);
		nbuf_MsgType_set_psize(&ms, psize);
		clear_padding_spaces();
	}
}

static void
cleanup()
{
	struct enumType *e;
	struct msgType *m;
	void *next;

	free(schema.pkgName);
	for (e = schema.enums; e; e = next) {
		struct enumDesc *d;
		free(e->name);
		for (d = e->vals; d; d = next) {
			free(d->name);
			next = d->next;
			free(d);
		}
		next = e->next;
		free(e);
	}
	for (m = schema.msgs; m; m = next) {
		struct fldDesc *d;
		free(m->name);
		for (d = m->flds; d; d = next) {
			free(d->name);
			free(d->tname);
			next = d->next;
			free(d);
		}
		next = m->next;
		free(m);
	}
}

/* TODO: if this is made reentrant, move it to lib/ */
void
nbuf_t2b(struct nbuf_buffer *buf)
{
	makelookup();
	makeschema(buf);
	outenums();
	outmsgs();
	cleanup();
}
