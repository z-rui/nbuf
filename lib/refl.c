#include "libnbuf.h"

#include <stdbool.h>
#include <string.h>

const char *
nbuf_EnumType_value_to_name(nbuf_EnumType *enm, uint16_t val)
{
	nbuf_EnumDesc d = nbuf_EnumType_values(enm, val);
	return nbuf_EnumDesc_name(&d, NULL);
}

bool
nbuf_Schema_msgType_by_name(
	nbuf_MsgType *msg,
	nbuf_Schema *schema,
	const char *name)
{
	size_t len = strlen(name);
	size_t n = nbuf_Schema_msgTypes_size(schema);
	size_t i;

	for (i = 0; i < n; i++) {
		*msg = nbuf_Schema_msgTypes(schema, i);
		size_t tname_len;
		const char *tname = nbuf_MsgType_name(msg, &tname_len);

		if (len == tname_len && memcmp(name, tname, len) == 0)
			return true;
	}
	return false;
}

bool
nbuf_Schema_enumType_by_name(
	nbuf_EnumType *enm,
	nbuf_Schema *schema,
	const char *name)
{
	size_t len = strlen(name);
	size_t n = nbuf_Schema_enumTypes_size(schema);
	size_t i;

	for (i = 0; i < n; i++) {
		*enm = nbuf_Schema_enumTypes(schema, i);
		size_t tname_len;
		const char *tname = nbuf_EnumType_name(enm, &tname_len);

		if (len == tname_len && memcmp(name, tname, len) == 0)
			return true;
	}
	return false;
}
