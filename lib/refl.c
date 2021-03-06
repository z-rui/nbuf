#include "libnbuf.h"

#include <assert.h>
#include <stdbool.h>
#include <string.h>

const char *
nbuf_enum_name(nbuf_EnumType enm, uint16_t val)
{
	size_t n = nbuf_EnumType_values_size(enm);
	size_t i;

	for (i = 0; i < n; i++) {
		nbuf_EnumDesc d = nbuf_EnumType_values(enm, i);
		if (val == nbuf_EnumDesc_value(d))
			return nbuf_EnumDesc_name(d, NULL);
	}
	return NULL;
}

uint32_t
nbuf_enum_value(nbuf_EnumType enm, const char *name)
{
	size_t len = strlen(name);
	size_t n = nbuf_EnumType_values_size(enm);
	size_t i;

	for (i = 0; i < n; i++) {
		nbuf_EnumDesc d = nbuf_EnumType_values(enm, i);
		size_t ename_len;
		const char *ename = nbuf_EnumDesc_name(d, &ename_len);

		if (len == ename_len && memcmp(name, ename, len) == 0)
			return nbuf_EnumDesc_value(d);
	}
	return -1;
}

bool
nbuf_find_msg(nbuf_MsgType *msg, nbuf_Schema schema, const char *name)
{
	size_t len = strlen(name);
	size_t n = nbuf_Schema_msgTypes_size(schema);
	size_t i;

	for (i = 0; i < n; i++) {
		*msg = nbuf_Schema_msgTypes(schema, i);
		size_t tname_len;
		const char *tname = nbuf_MsgType_name(*msg, &tname_len);

		if (len == tname_len && memcmp(name, tname, len) == 0)
			return true;
	}
	return false;
}

bool
nbuf_find_enum(nbuf_EnumType *enm, nbuf_Schema schema, const char *name)
{
	size_t len = strlen(name);
	size_t n = nbuf_Schema_enumTypes_size(schema);
	size_t i;

	for (i = 0; i < n; i++) {
		*enm = nbuf_Schema_enumTypes(schema, i);
		size_t tname_len;
		const char *tname = nbuf_EnumType_name(*enm, &tname_len);

		if (len == tname_len && memcmp(name, tname, len) == 0)
			return true;
	}
	return false;
}

bool
nbuf_find_field(nbuf_FieldDesc *fld, nbuf_MsgType msg, const char *name)
{
	size_t len = strlen(name);
	size_t n = nbuf_MsgType_fields_size(msg);
	size_t i;

	for (i = 0; i < n; i++) {
		*fld = nbuf_MsgType_fields(msg, i);
		size_t fname_len;
		const char *fname = nbuf_FieldDesc_name(*fld, &fname_len);

		if (len == fname_len && memcmp(name, fname, len) == 0)
			return true;
	}
	return false;
}

extern void
_nbuf_get_size(struct nbuf_obj *o, nbuf_Schema schema, nbuf_Kind kind, uint32_t tag1)
{
	nbuf_MsgType msg;

	switch (kind) {
	case nbuf_Kind_ENUM:
		o->ssize = 2;
		o->psize = 0;
		break;
	case nbuf_Kind_BOOL:
	case nbuf_Kind_INT:
	case nbuf_Kind_UINT:
	case nbuf_Kind_FLOAT:
		o->ssize = tag1;
		o->psize = 0;
		break;
	case nbuf_Kind_STR:
		o->ssize = 0;
		o->psize = 1;
		break;
	case nbuf_Kind_PTR:
		msg = nbuf_Schema_msgTypes(schema, tag1);
		o->ssize = nbuf_MsgType_ssize(msg);
		o->psize = nbuf_MsgType_psize(msg);
		break;
	default:
		assert(0 && "bad kind");
		break;
	}
}
