#ifndef B2H_COMMON_H
#define B2H_COMMON_H

/* For b2h internal use */

#include "nbuf.nb.h"
#include <stdio.h>
#include <ctype.h>

struct nbuf_b2h {
	nbuf_Schema schema;
	char *prefix, *guard;
	char scratch[32];
};

static inline const char *
maybeprefix(struct nbuf_b2h *ctx, nbuf_Kind kind)
{
	if (kind == nbuf_Kind_ENUM || kind == nbuf_Kind_PTR)
		return ctx->prefix;
	return "";
}

#define typestr nbuf_b2h_typestr
#define makeprefix nbuf_b2h_makeprefix

extern const char *
typestr(struct nbuf_b2h *ctx, nbuf_Kind kind, uint32_t tag1);
extern void
makeprefix(struct nbuf_b2h *ctx, const char *srcname,
	const char *prefix_suffix, const char *guard_suffix);

#endif  /* B2H_COMMON_H */
