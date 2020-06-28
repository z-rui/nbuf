#ifndef B2H_COMMON_H
#define B2H_COMMON_H

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

static inline const char *
typestr(struct nbuf_b2h *ctx, nbuf_Kind kind, uint32_t tag1)
{
	switch (kind) {
	case nbuf_Kind_BOOL:
		return "bool";
	case nbuf_Kind_INT:
		sprintf(ctx->scratch, "int%d_t", tag1*8);
		return ctx->scratch;
	case nbuf_Kind_UINT:
		sprintf(ctx->scratch, "uint%d_t", tag1*8);
		return ctx->scratch;
	case nbuf_Kind_FLOAT:
		return (tag1 == 4) ? "float" : "double";
	case nbuf_Kind_STR:
		/* TODO: use std::string_view in C++ */
		return "const char *";
	case nbuf_Kind_ENUM:
		return nbuf_EnumType_name(nbuf_Schema_enumTypes(ctx->schema, tag1), NULL);
	case nbuf_Kind_PTR:
		return nbuf_MsgType_name(nbuf_Schema_msgTypes(ctx->schema, tag1), NULL);
	default:
		assert(0 && "unknown kind");
		break;
	}
	return NULL;
}

static inline struct nbuf_obj
getsizeinfo(struct nbuf_b2h *ctx, nbuf_Kind kind, uint32_t tag1)
{
	struct nbuf_obj o = {0};

	switch (kind) {
	case nbuf_Kind_BOOL:
		break;
	case nbuf_Kind_INT:
	case nbuf_Kind_UINT:
	case nbuf_Kind_FLOAT:
		o.ssize = tag1;
		break;
	case nbuf_Kind_ENUM:
		o.ssize = 2;
		break;
	case nbuf_Kind_STR:
		o.psize = 1;
		break;
	case nbuf_Kind_PTR: {
		nbuf_MsgType msg = nbuf_Schema_msgTypes(ctx->schema, tag1);
		o.ssize = nbuf_MsgType_ssize(msg);
		o.psize = nbuf_MsgType_psize(msg);
		break;
	}
	default:
		assert(0 && "bad kind");
	}
	return o;
}

static inline void
makeprefix(struct nbuf_b2h *ctx, const char *srcname, const char *suffix)
{
	size_t i, len;
	const char *pkgName = nbuf_Schema_pkgName(ctx->schema, &len);
	char ch;

	if (len == 0) {
		ctx->prefix = strdup("");
		len = strlen(srcname);
		/* file.nb => FILE_NB_H */
		ctx->guard = malloc(len + strlen(suffix) + 1);
		for (i = 0; i < len; i++) {
			ch = srcname[i];
			if (!isalnum(ch))
				ch = '_';
			ctx->guard[i] = toupper(ch);
		}
		strcpy(ctx->guard + len, suffix);
	} else {
		/* pkg.name => pkg_name_, PKG_NAME_NB_H */
		ctx->prefix = malloc(len + 2);
		ctx->guard = malloc(len + 3 + strlen(suffix));
		for (i = 0; i < len; i++) {
			ch = pkgName[i];
			if (!isalnum(ch))
				ch = '_';
			ctx->prefix[i] = ch;
			ctx->guard[i] = toupper(ch);
		}
		strcpy(ctx->prefix + len, "_");
		strcpy(ctx->guard + len, "_NB");
		strcpy(ctx->guard + len + 3, suffix);
	}
}

#endif  /* B2H_COMMON_H */
