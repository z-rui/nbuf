#include "libnbuf.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

struct pool_buffer {
	struct nbuf_buffer buf;
	struct nbuf_obj o;
	uint32_t id;
};

struct nbuf_obj
nbuf_pool_create(struct nbuf_pool *pool,
	uint32_t nelem, uint16_t ssize, uint16_t psize)
{
	struct nbuf_obj o = {NULL};
	struct pool_buffer *buf;

	if (pool->len == pool->cap) {
		size_t cap = (pool->cap) ? pool->cap * 2 : 1;
		struct nbuf_buffer **buffers = (struct nbuf_buffer **)
			realloc(pool->buffers, cap * sizeof *pool->buffers);

		if (buffers == NULL)
			return o;
		pool->cap = cap;
		pool->buffers = buffers;
	}
	buf = malloc(sizeof *buf);
	if (buf == NULL)
		return o;
	buf->id = pool->len;
	pool->buffers[pool->len++] = &buf->buf;
	nbuf_init_write(&buf->buf, NULL, 0);
	o = nbuf_create(&buf->buf, nelem, ssize, psize);
	return o;
}

struct nbuf_obj
nbuf_get_pool_ptr(const struct nbuf_pool *pool,
	const struct nbuf_obj *r, size_t i)
{
	struct nbuf_obj rr = {r->buf};

	if (i >= r->psize)
		return rr;
	size_t base = r->base + nbuf_ptr_offs(r, i);
	int64_t j = nbuf_read_int_safe(r->buf, base, NBUF_WORD_SZ);
	assert(j >= 0 && (size_t) j < pool->len);
	rr.buf = pool->buffers[j];
	nbuf_obj_init(&rr, 0);
	return rr;
}

void
nbuf_put_pool_ptr(struct nbuf_pool *pool,
	struct nbuf_obj *r, size_t i, struct nbuf_obj v)
{
	size_t ptr_base;
	struct pool_buffer *buf = (struct pool_buffer *) v.buf;

	assert(buf->id < pool->len);
	ptr_base = r->base + nbuf_ptr_offs(r, i);
	nbuf_write_int_safe(r->buf, ptr_base, NBUF_WORD_SZ, buf->id);
}

struct nbuf_obj
nbuf_pool_serialize(struct nbuf_buffer *buf, const struct nbuf_pool *pool)
{
	struct nbuf_obj o = {buf};
	size_t i, j, k;

	/* 1st pass - copy all objects */
	for (i = 0; i < pool->len; i++) {
		struct nbuf_obj o = {pool->buffers[i]};
		struct nbuf_obj *oo = &((struct pool_buffer *) pool->buffers[i])->o;
		size_t elemsz, totalsz;

		nbuf_obj_init(&o, 0);
		elemsz = nbuf_elemsz(&o);
		totalsz = (elemsz == 0) ? (o.nelem + 7) / 8 : elemsz * o.nelem;
		*oo = nbuf_create(buf, o.nelem, o.ssize, o.psize);
		if (oo->base + totalsz > buf->len)
			return o;
		memcpy(buf->base + oo->base, o.buf->base + o.base, totalsz);
	}
	/* 2nd pass - fix pointers */
	for (i = 0; i < pool->len; i++) {
		struct nbuf_obj o = ((struct pool_buffer *) pool->buffers[i])->o;
		size_t elemsz = nbuf_elemsz(&o);

		for (j = 0; j < o.nelem; j++) {
			for (k = 0; k < o.psize; k++) {
				size_t base = o.base + nbuf_ptr_offs(&o, k);
				int64_t buf_idx = nbuf_read_int_safe(o.buf, base, NBUF_WORD_SZ);
				if (buf_idx) {
					assert(buf_idx > i && (size_t) buf_idx < pool->len);
					struct nbuf_obj oo = ((struct pool_buffer *) pool->buffers[buf_idx])->o;
					nbuf_put_ptr(&o, k, oo);
				}
			}
			o.base += elemsz;
		}
	}
	if (pool->len > 0)
		o = ((struct pool_buffer *) *pool->buffers)->o;
	return o;
}

void
nbuf_pool_free(struct nbuf_pool *pool)
{
	size_t i;

	for (i = 0; i < pool->len; i++) {
		nbuf_free(pool->buffers[i]);
		free(pool->buffers[i]);
	}
	free(pool->buffers);
}
