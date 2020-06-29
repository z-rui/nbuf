#ifndef NBUF_H
#define NBUF_H

#include <assert.h>
#include <limits.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define NBUF_WORD_SZ ((size_t) 4)
#define NBUF_HALF_WORD_BIT 16
#define NBUF_WORD_BIT 32
#define NBUF_ALIGN 8
#define NBUF_ALLOC_MIN 64
#define NBUF_ROUNDUP(x, n) (((x)+(n-1))/(n)*(n))

static inline  int64_t
nbuf_read_int_unsafe(const void *ptr, size_t sz)
{
#ifdef NBUF_BIG_ENDIAN
	switch (sz) {
	case 8: return __builtin_bswap64(*(const int64_t *) ptr);
	case 4: return __builtin_bswap32(*(const int32_t *) ptr);
	case 2: return __builtin_bswap16(*(const int16_t *) ptr);
	case 1: return *(const int8_t *) ptr;
	}
#else
	switch (sz) {
	case 8: return *(const int64_t *) ptr;
	case 4: return *(const int32_t *) ptr;
	case 2: return *(const int16_t *) ptr;
	case 1: return *(const int8_t *) ptr;
	}
#endif
	assert(0 && "bad scalar size");
	return 0;
}

struct nbuf_buffer {
	char *base;
	size_t len, cap;
	struct nbuf_ops *ops;
};

struct nbuf_ops {
	struct nbuf_obj (*create)(struct nbuf_buffer *, uint32_t, uint16_t, uint16_t);
	struct nbuf_obj (*get_ptr)(const struct nbuf_obj *, size_t);
	void (*put_ptr)(const struct nbuf_obj *, size_t, struct nbuf_obj);
};

static inline void
nbuf_init_read(struct nbuf_buffer *buf, const void *base, size_t size)
{
	buf->base = (char *) base;
	buf->len = size;
	buf->cap = 0;  // for assert to catch writing to read buffers
	buf->ops = NULL;
}

static inline bool
nbuf_init_write(struct nbuf_buffer *buf, const void *base, size_t size)
{
	buf->base = (char *) ((base) ? base : (size) ? malloc(size) : NULL);
	buf->len = 0;
	buf->cap = size;
	buf->ops = NULL;
	return buf->base != NULL;
}

static inline bool
nbuf_reserve(struct nbuf_buffer *buf, size_t inc)
{
	size_t cap = buf->cap;
	assert(cap >= buf->len);
	if (cap == 0)
		cap = NBUF_ALLOC_MIN;
	while (inc > cap - buf->len)
		if (!(cap *= 2))
			return false;
	if (cap > buf->cap) {
		char *base;
		if ((base = (char *) realloc(buf->base, cap)) == NULL)
			return false;
		buf->base = base;
		buf->cap = cap;
	}
	return true;
}

static inline void
nbuf_clear(struct nbuf_buffer *buf)
{
	buf->len = 0;
}

static inline void
nbuf_free(struct nbuf_buffer *buf)
{
	free(buf->base);
	buf->base = NULL;
	buf->len = buf->cap = 0;
}

static inline bool
nbuf_bounds_check(
	const struct nbuf_buffer *buf,
	size_t byte_offset,
	size_t sz)
{
	return byte_offset <= buf->len && byte_offset + sz <= buf->len;
}

static inline bool
nbuf_writable(const struct nbuf_buffer *buf)
{
	return buf->cap >= buf->len;
}

static inline int64_t
nbuf_read_int_safe(
	const struct nbuf_buffer *buf, size_t base, size_t sz)
{
	assert(nbuf_bounds_check(buf, base, sz));
	return nbuf_read_int_unsafe(buf->base + base, sz);
}

struct nbuf_obj {
	struct nbuf_buffer *buf; /* for bounds-checking */
	size_t base;         /* byte offset from buffer start */
	uint32_t nelem;      /* message: 1; vector:  # of elements */
	uint16_t ssize;      /* size of scalar part, in bytes; 0 if it's a bit. */
	uint16_t psize;      /* size of pointer part, in words */
};

static inline size_t
nbuf_ptr_offs(const struct nbuf_obj *r, size_t i)
{
	return r->ssize + i * NBUF_WORD_SZ;
}

static inline size_t
nbuf_elemsz(const struct nbuf_obj *r)
{
	return nbuf_ptr_offs(r, r->psize);
}

static inline size_t
nbuf_totalsz(const struct nbuf_obj *r)
{
	size_t elemsz = nbuf_elemsz(r);
	return (elemsz) ? elemsz * r->nelem : (r->nelem + 7) / 8;
}

static inline void
nbuf_obj_init(
	struct nbuf_obj *rr, /* in: buf, out: others */
	size_t base)
{
	uint64_t hdr;

#ifndef NBUF_UNSAFE
	/* can't trust the address */
	if (!nbuf_bounds_check(rr->buf, base, sizeof hdr))
		return;
#endif
	hdr = nbuf_read_int_safe(rr->buf, base, sizeof hdr);
	base += sizeof hdr;
	rr->base = base;
	rr->nelem = hdr & ((1ULL<<NBUF_WORD_BIT)-1);
	hdr >>= NBUF_WORD_BIT;
	rr->ssize = hdr & ((1<<NBUF_HALF_WORD_BIT)-1);
	rr->psize = hdr >> NBUF_HALF_WORD_BIT;

	if (rr->nelem > 0) {
#ifndef NBUF_UNSAFE
		size_t elemsz = nbuf_elemsz(rr);
		size_t totalsz = (elemsz == 0)
			? (rr->nelem + 7) / 8 : elemsz * rr->nelem;
		if (totalsz == 0 || !nbuf_bounds_check(rr->buf, base, totalsz)) {
			/* corrupted/malicious message */
			rr->base = rr->nelem = rr->ssize = rr->psize = 0;
			return;
		}
#endif
	}
}

static inline struct nbuf_obj
nbuf_get_elem(const struct nbuf_obj *r, size_t i)
{
	struct nbuf_obj rr = {r->buf};
	size_t elemsz = nbuf_elemsz(r);
	if (i < r->nelem) {
		rr.base = r->base + ((elemsz == 0) ? i / 8 :i * elemsz);
		rr.nelem = 1;
		rr.ssize = (elemsz == 0) ? 1 : r->ssize;
		rr.psize = r->psize;
	}
	return rr;
}

static inline size_t
nbuf_get_rel_ptr(const struct nbuf_obj *r, size_t i)
{
	if (i >= r->psize)
		return 0;
	return nbuf_read_int_safe(r->buf,
		r->base + nbuf_ptr_offs(r, i),
		NBUF_WORD_SZ);
}

static inline bool
nbuf_has_ptr(const struct nbuf_obj *r, size_t i)
{
	return nbuf_get_rel_ptr(r, i) != 0;
}

static inline struct nbuf_obj
_nbuf_get_ptr(const struct nbuf_obj *r, size_t i)
{
	struct nbuf_obj rr = {r->buf};
	size_t base = nbuf_get_rel_ptr(r, i) * NBUF_WORD_SZ;
	if (base) {
		base += r->base + nbuf_ptr_offs(r, i);
		if (base < r->buf->len)
			nbuf_obj_init(&rr, base);
	}
	return rr;
}

static inline struct nbuf_obj
nbuf_get_ptr(const struct nbuf_obj *r, size_t i)
{
#ifndef NBUF_NO_POLYMORPHISM
	if (r->buf->ops)
		return r->buf->ops->get_ptr(r, i);
#endif
	return _nbuf_get_ptr(r, i);
}

static inline int64_t
nbuf_get_int(const struct nbuf_obj *r, size_t byte_offset, size_t sz)
{
	assert(byte_offset % sz == 0);
	return (byte_offset + sz <= r->ssize) ?
		nbuf_read_int_safe(r->buf, r->base + byte_offset, sz) : 0;
}

static inline float
nbuf_get_f32(const struct nbuf_obj *r, size_t byte_offset)
{
	union { int32_t i; float f; } u;
	assert(sizeof u.f == sizeof u.i);
	u.i = nbuf_get_int(r, byte_offset, sizeof u);
	return u.f;
}

static inline double
nbuf_get_f64(const struct nbuf_obj *r, size_t byte_offset)
{
	union { int64_t i; double f; } u;
	assert(sizeof u.f == sizeof u.i);
	u.i = nbuf_get_int(r, byte_offset, sizeof u);
	return u.f;
}

static inline bool
nbuf_get_bit(const struct nbuf_obj *r, size_t bit_offset)
{
	size_t i = bit_offset / CHAR_BIT;
	size_t j = bit_offset % CHAR_BIT;
	unsigned char b = nbuf_get_int(r, i, 1);
	return !!(b & (1<<j));
}

static inline char *
nbuf_get_str(const struct nbuf_obj *r, size_t i, size_t *lenp)
{
	char *s = (char *) "";
	size_t len = 0;
	struct nbuf_obj rr = nbuf_get_ptr(r, i);
	if (rr.nelem > 0) {
		assert(nbuf_bounds_check(r->buf, rr.base + rr.nelem - 1, 1));
		len = rr.nelem - 1;
		s = rr.buf->base + rr.base;
	}
	if (lenp != NULL)
		*lenp = len;
	return s;
}

static inline void
nbuf_write_int_unsafe(void *ptr, size_t sz, uint64_t v)
{
#ifdef NBUF_BIG_ENDIAN
	switch (sz) {
	case 8: *(int64_t *) ptr = __builtin_bswap64(v); return;
	case 4: *(int32_t *) ptr = __builtin_bswap32(v); return;
	case 2: *(int16_t *) ptr = __builtin_bswap16(v); return;
	case 1: *(int8_t *) ptr = v; return;
	}
#else
	switch (sz) {
	case 8: *(int64_t *) ptr = v; return;
	case 4: *(int32_t *) ptr = v; return;
	case 2: *(int16_t *) ptr = v; return;
	case 1: *(int8_t *) ptr = v; return;
	}
#endif
	assert(0 && "bad scalar size");
}

static inline void
nbuf_write_int_safe(
	const struct nbuf_buffer *buf, size_t base, size_t sz, int64_t v)
{
	assert(nbuf_bounds_check(buf, base, sz));
	return nbuf_write_int_unsafe(buf->base + base, sz, v);
}

/* The writer is more trusted than the read buffer.
 * Invalid values will be checked by assert rather than
 * return a safe value.
 */
static inline struct nbuf_obj
_nbuf_create(struct nbuf_buffer *buf,
	uint32_t nelem,
	uint16_t ssize,
	uint16_t psize) {
	 /* buf pointers may be updated in case of reallocation. */
	uint64_t hdr = nelem
		| (((uint64_t) ssize) << 32)
		| (((uint64_t) psize) << 48);
	struct nbuf_obj rr = {buf, buf->len, nelem, ssize, psize};
	size_t elemsz = nbuf_elemsz(&rr);
	size_t totalsz;

	totalsz = sizeof hdr;
	totalsz += (ssize == 0 && psize == 0)
		? (nelem+7) / 8  /* bool array */
		: nelem * elemsz;  /* other */
	totalsz = NBUF_ROUNDUP(totalsz, NBUF_ALIGN);

	assert(buf->len <= buf->cap);
	/* since {s,p}size are 16 bits,
	 * elemsz should be reasonably small and the multiplication
	 * cannot overflow. */
	assert(0 <= elemsz && elemsz < UINT32_MAX);
	assert(elemsz == 0 || SIZE_MAX / elemsz > nelem);
	/* pointers must be properly aligned */
	assert(psize == 0 || elemsz % NBUF_WORD_SZ == 0);
	if (!nbuf_reserve(buf, totalsz)) {
		rr.nelem = rr.ssize = rr.psize = 0;
		return rr;
	}
	assert(totalsz <= buf->cap - buf->len);
	assert(buf->len % (sizeof hdr) == 0);
	buf->len += totalsz;
	nbuf_write_int_safe(rr.buf, rr.base, sizeof hdr, hdr);
	rr.base += sizeof hdr;
	memset(buf->base + rr.base, 0, totalsz - sizeof hdr);
	return rr;
}

static inline struct nbuf_obj
nbuf_create(struct nbuf_buffer *buf,
	uint32_t nelem,
	uint16_t ssize,
	uint16_t psize) {
	if (buf->ops)
		return buf->ops->create(buf, nelem, ssize, psize);
	return _nbuf_create(buf, nelem, ssize, psize);
}

static inline void
nbuf_resize(struct nbuf_obj *r, size_t nelem)
{
	size_t elemsz = nbuf_elemsz(r);
	size_t end, newend;

	end = newend = r->base;
	if (elemsz != 0) {
		end += r->nelem * elemsz;
		newend += nelem * elemsz;
	} else {
		end += (r->nelem+7) / 8;
		newend += (nelem+7) / 8;
	}
	end = NBUF_ROUNDUP(end, NBUF_ALIGN);
	newend = NBUF_ROUNDUP(newend, NBUF_ALIGN);
	assert(nelem < UINT32_MAX);
	if (end != r->buf->len)
		/* resizing an object not at the end of the buffer
		 * is not supported. */
		return;
	if (newend > end && !nbuf_reserve(r->buf, newend - end))
		return;
	assert(newend <= r->buf->cap);
	memset(r->buf->base + end, 0, newend - end);
	r->buf->len = newend;
	r->nelem = nelem;
	nbuf_write_int_safe(r->buf, r->base - NBUF_WORD_SZ*2, NBUF_WORD_SZ, nelem);
}

static inline void
nbuf_put_int(const struct nbuf_obj *r, size_t byte_offset, size_t sz, uint64_t v)
{
	assert(nbuf_writable(r->buf));
	assert(byte_offset % sz == 0);
	if (byte_offset + sz <= r->ssize)
		nbuf_write_int_safe(r->buf, r->base + byte_offset, sz, v);
}

static inline void
nbuf_put_f32(const struct nbuf_obj *r, size_t byte_offset, float f)
{
	union { int32_t i; float f; } u;
	assert(sizeof u.f == sizeof u.i);
	u.f = f;
	nbuf_put_int(r, byte_offset, sizeof u, u.i);
}

static inline void
nbuf_put_f64(const struct nbuf_obj *r, size_t byte_offset, double f)
{
	union { int64_t i; double f; } u;
	assert(sizeof u.f == sizeof u.i);
	u.f = f;
	nbuf_put_int(r, byte_offset, sizeof u, u.i);
}

static inline void
nbuf_put_bit(const struct nbuf_obj *r, size_t bit_offset, bool value)
{
	size_t i = bit_offset / CHAR_BIT;
	size_t j = bit_offset % CHAR_BIT;
	if (i >= r->ssize)
		return;
	size_t k = r->base + i;
	assert(nbuf_bounds_check(r->buf, k, 1));
	if (value)
		r->buf->base[k] |= (1U<<j);
	else
		r->buf->base[k] &= ~(1U<<j);
}

static inline void
_nbuf_put_ptr(const struct nbuf_obj *r, size_t i, struct nbuf_obj v)
{
	assert(r->buf == v.buf);
	if (i >= r->psize)
		return;
	size_t ptr_base = r->base + nbuf_ptr_offs(r, i);
	size_t hdr_base = v.base - NBUF_ALIGN;
	assert(nbuf_bounds_check(r->buf, hdr_base, NBUF_ALIGN));
	int64_t rel = hdr_base - ptr_base;
	assert(rel > 0 && rel % NBUF_WORD_SZ == 0);
	nbuf_write_int_safe(r->buf, ptr_base, NBUF_WORD_SZ, rel / NBUF_WORD_SZ);
}

static inline void
nbuf_put_ptr(const struct nbuf_obj *r, size_t i, struct nbuf_obj v)
{
	if (r->buf->ops)
		return r->buf->ops->put_ptr(r, i, v);
	return _nbuf_put_ptr(r, i, v);
}

static inline void
nbuf_put_str(const struct nbuf_obj *r, size_t i, const char *s, size_t len)
{
	if (len+1 == 0)
		len = strlen(s);
	struct nbuf_obj rr = nbuf_create(r->buf, len+1, 1, 0);
	if (rr.nelem == 0)
		return;
	assert(rr.nelem == len + 1);
	char *base = rr.buf->base + rr.base;
	memcpy(base, s, len);
	base[len] = '\0';
	nbuf_put_ptr(r, i, rr);
}

#endif  /* NBUF_H */
