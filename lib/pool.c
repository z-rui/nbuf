#include "libnbuf.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

struct objmeta {
	struct nbuf_buffer *pool;
	uint32_t offset;  /* in words */
	uint32_t id;
};

static struct nbuf_buffer *
getpool(struct nbuf_buffer *buf)
{
	return *(struct nbuf_buffer **) buf->base;
}

static struct nbuf_obj
getobj(struct nbuf_buffer *buf)
{
	struct nbuf_obj o = {buf};
	nbuf_obj_init(&o, NBUF_ROUNDUP(sizeof (struct objmeta), NBUF_ALIGN));
	return o;
}

static struct objmeta *
getmeta(struct nbuf_buffer *buf)
{
	return (struct objmeta *) buf->base;
}

static struct nbuf_buffer *
getbuffer(struct nbuf_buffer *pool, size_t i)
{
	assert((i+1) * sizeof (struct nbuf_buffer *) < pool->len);
	return ((struct nbuf_buffer **) pool->base)[i+1];
}

static void
putbuffer(struct nbuf_buffer *pool, size_t i, struct nbuf_buffer *buf)
{
	assert((i+1) * sizeof (struct nbuf_buffer *) < pool->len);
	((struct nbuf_buffer **) pool->base)[i+1] = buf;
}

static size_t
buffercount(struct nbuf_buffer *pool)
{
	return pool->len / sizeof (struct buffer *) - 1;
}

static struct nbuf_obj
get_ptr(const struct nbuf_obj *r, size_t i)
{
	struct nbuf_buffer *pool = getpool(r->buf);
	struct nbuf_obj rr = {r->buf};

	if (i >= r->psize)
		return rr;
	size_t base = r->base + nbuf_ptr_offs(r, i);
	int64_t j = nbuf_read_int_safe(r->buf, base, NBUF_WORD_SZ);
	if (j == 0)
		return rr;
	assert(j > 0 && (size_t) j < buffercount(pool));
	return getobj(getbuffer(pool, j));
}

static void
put_ptr(const struct nbuf_obj *r, size_t i, struct nbuf_obj v)
{
	struct nbuf_buffer *pool = getpool(r->buf);
	const struct objmeta *meta = getmeta(v.buf);

	assert(pool == meta->pool);
	assert(meta->id < buffercount(pool));
	size_t ptr_base = r->base + nbuf_ptr_offs(r, i);
	nbuf_write_int_safe(r->buf, ptr_base, NBUF_WORD_SZ, meta->id);
}

static struct nbuf_obj
create(struct nbuf_buffer *, uint32_t, uint16_t, uint16_t);

struct nbuf_ops builder_ops = {
	.create = create,
	.get_ptr = get_ptr,
	.put_ptr = put_ptr,
};

static struct nbuf_obj
create(struct nbuf_buffer *buf,
	uint32_t nelem, uint16_t ssize, uint16_t psize)
{
	struct nbuf_obj o = {NULL};
	struct nbuf_buffer *pool = getpool(buf);
	struct nbuf_buffer *b;
	struct objmeta *meta;

	if (!nbuf_reserve(pool, sizeof (struct buffer *)))
		return o;
	if ((b = malloc(sizeof *b)) == NULL)
		return o;

	b->base = NULL;
	b->len = 0;
	b->cap = 0;
	b->ops = &builder_ops;

	if (!nbuf_reserve(b, NBUF_ROUNDUP(sizeof *meta, NBUF_ALIGN))) {
		free(b);
		return o;
	}
	b->len += sizeof *meta;

	/* create obj before writing meta since the former may realloc */
	o = _nbuf_create(b, nelem, ssize, psize);
	meta = getmeta(b);
	meta->pool = pool;
	meta->id = buffercount(pool);
	pool->len += sizeof (struct nbuf_buffer *);
	putbuffer(pool, meta->id, b);

	return o;
}

bool
nbuf_serialize(struct nbuf_buffer *buf)
{
	const size_t kHdrSize = 8;
	struct nbuf_buffer *pool = getpool(buf);
	size_t i, j, k, n;
	size_t cursor = 0;
	char *output;

	assert(pool == buf);
	n = buffercount(pool);

	/* 1st pass - calculate all offsets */
	for (i = 0; i < n; i++) {
		struct nbuf_buffer *b = getbuffer(pool, i);
		struct nbuf_obj o = getobj(b);
		struct objmeta *meta = getmeta(b);
		meta->offset = cursor / NBUF_WORD_SZ;
		cursor = NBUF_ROUNDUP(cursor + kHdrSize + nbuf_totalsz(&o), NBUF_ALIGN);
	}

	output = malloc(cursor);
	if (!output)
		return false;

	/* 2nd pass - copy objects and fix pointers */
	cursor = 0;
	for (i = 0; i < n; i++) {
		struct nbuf_buffer *b = getbuffer(pool, i);
		struct nbuf_obj o = getobj(b);
		const char *src = b->base + o.base;

		assert(cursor == getmeta(b)->offset * NBUF_WORD_SZ);
		memcpy(output + cursor, src - kHdrSize, kHdrSize);
		cursor += kHdrSize;
		if (o.psize == 0) {
			size_t totalsz = nbuf_totalsz(&o);
			memcpy(output + cursor, src, totalsz);
			cursor += totalsz;
		} else {
			for (j = 0; j < o.nelem; j++) {
				memcpy(output + cursor, src, o.ssize);
				cursor += o.ssize;
				src += o.ssize;
				for (k = 0; k < o.psize; k++) {
					size_t id = nbuf_read_int_unsafe(src, NBUF_WORD_SZ);
					size_t rel = 0;
					src += NBUF_WORD_SZ;
					if (id) {
						assert(id > i && id < n);
						struct objmeta *meta = getmeta(getbuffer(pool, id));
						rel = meta->offset - cursor / NBUF_WORD_SZ;
					}
					nbuf_write_int_unsafe(output + cursor, NBUF_WORD_SZ, rel);
					cursor += NBUF_WORD_SZ;
				}
			}
		}
		while (cursor % NBUF_ALIGN)
			output[cursor++] = 0;
	}

	nbuf_free_builder(buf);

	/* revert to a normal buffer */
	buf->base = output;
	buf->len = buf->cap = cursor;
	buf->ops = NULL;

	return true;
}

extern bool nbuf_init_builder(struct nbuf_buffer *buf, size_t size)
{
	buf->base = NULL;
	buf->len = 0;
	buf->cap = 0;
	if (!nbuf_reserve(buf, (size + 1) * sizeof (struct nbuf_buffer *)))
		return false;
	*(struct nbuf_buffer **) buf->base = buf;
	buf->len += sizeof buf;
	buf->ops = &builder_ops;
	return true;
}

extern void nbuf_free_builder(struct nbuf_buffer *buf)
{
	struct nbuf_buffer *pool = getpool(buf);
	size_t n, i;

	assert(pool == buf);
	n = buffercount(pool);

	for (i = 0; i < n; i++) {
		struct nbuf_buffer *b = getbuffer(pool, i);
		nbuf_free(b);
		free(b);
	}
	nbuf_free(pool);
}
