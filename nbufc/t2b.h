#ifndef T2B_H
#define T2B_H

#include "nbuf.h"

#include <stdbool.h>
#include <stdint.h>

struct enumType {
	char *name;
	struct enumDesc *vals;
	struct enumType *next;
};

struct enumDesc {
	char *name;
	uint16_t val;
	struct enumDesc *next;
};

struct msgType {
	char *name;
	struct fldDesc *flds;
	struct msgType *next;
};

struct fldDesc {
	char *name;
	char *tname;
	bool list;
	struct fldDesc *next;
};

struct schemaDesc {
	char *pkgName;
	struct enumType *enums;
	struct msgType *msgs;
};

extern struct schemaDesc schema;

extern void nbuf_t2b(struct nbuf_buffer *buf);

#endif  /* T2B_H */
