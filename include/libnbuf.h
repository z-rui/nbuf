#ifndef LIBNBUF_H
#define LIBNBUF_H

#include "nbuf.h"
#include "nbuf.nb.h"

#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

extern const char *
nbuf_EnumType_value_to_name(nbuf_EnumType *enm, uint16_t val);

extern bool
nbuf_Schema_msgType_by_name(
	nbuf_MsgType *msg,
	nbuf_Schema *schema,
	const char *name);

extern bool
nbuf_Schema_enumType_by_name(
	nbuf_EnumType *enm,
	nbuf_Schema *schema,
	const char *name);

extern int
nbuf_print(struct nbuf_obj *o, FILE *fout, int indent,
	nbuf_Schema *schema, nbuf_MsgType *msgType);

#ifdef __cplusplus
}
#endif

#endif  /* LIBNBUF_H */
