/* Generate a binary schema
 *
 * This requires an old version of nbuf.nb.h
 * for bootstrapping.
 */

#include "nbuf.h"
#include "bootstrap.h"

#include <stdio.h>
#include <stdarg.h>

void
set_msgtype(msgType, name, nfields, ssize, psize)
	nbuf_MsgType *msgType;
	const char *name;
	size_t nfields, ssize, psize;
{
	nbuf_MsgType_set_name(msgType, name, -1);
	nbuf_MsgType_init_fields(msgType, nfields);
	nbuf_MsgType_set_ssize(msgType, ssize);
	nbuf_MsgType_set_psize(msgType, psize);
}

void
set_field(msgType, idx, name, kind, list, tag0, tag1)
	nbuf_MsgType *msgType;
	size_t idx;
	const char *name;
	nbuf_Kind kind;
	bool list;
	uint32_t tag0, tag1;
{
	nbuf_FieldDesc field;

	field = nbuf_MsgType_fields(msgType, idx);
	nbuf_FieldDesc_set_name(&field, name, -1);
	nbuf_FieldDesc_set_kind(&field, kind);
	nbuf_FieldDesc_set_list(&field, list);
	nbuf_FieldDesc_set_tag0(&field, tag0);
	nbuf_FieldDesc_set_tag1(&field, tag1);
}

void set_enum(nbuf_EnumType *enumType, const char *name,
	size_t n, ...)
{
	va_list ap;
	int i;

	nbuf_EnumType_set_name(enumType, name, -1);
	nbuf_EnumType_init_values(enumType, n);
	assert(nbuf_EnumType_values_size(enumType) == n);
	va_start(ap, n);
	for (i = 0; i < n; i++) {
		const char *name = va_arg(ap, const char *);
		nbuf_Kind value = va_arg(ap, nbuf_Kind);
		nbuf_EnumDesc enumDesc = nbuf_EnumType_values(enumType, i);
		nbuf_EnumDesc_set_name(&enumDesc, name, -1);
		nbuf_EnumDesc_set_value(&enumDesc, (uint16_t) value);
	}
	va_end(ap);
}

void
make_schema(struct nbuf_buffer *buf)
{
	nbuf_Schema schema;
	nbuf_MsgType msgType;
	nbuf_EnumType enumType;

	schema = nbuf_new_nbuf_Schema(buf);
	nbuf_Schema_set_pkgName(&schema, "nbuf", 4);

	/* msgTypes:
	 * 0 Schema,
	 * 1 EnumDesc,
	 * 2 EnumType,
	 * 3 FieldDesc,
	 * 4 MsgType */
	nbuf_Schema_init_msgTypes(&schema, 5);

	/* Schema */
	msgType = nbuf_Schema_msgTypes(&schema, 0);
	set_msgtype(&msgType, "Schema", 3, 0, 3);
	set_field(&msgType, 0, "msgTypes", nbuf_Kind_PTR,
		/*list=*/true, /*idx=*/0, 4/*MsgType*/);
	set_field(&msgType, 1, "enumTypes", nbuf_Kind_PTR,
		/*list=*/true, /*idx=*/1, 2/*EnumType*/);
	set_field(&msgType, 2, "pkgName", nbuf_Kind_STR,
		/*list=*/false, /*idx=*/2, 0);

	/* EnumDesc */
	msgType = nbuf_Schema_msgTypes(&schema, 1);
	set_msgtype(&msgType, "EnumDesc", 2, 4, 1);
	set_field(&msgType, 0, "name", nbuf_Kind_STR,
		/*list=*/false, /*idx=*/0, 0);
	set_field(&msgType, 1, "value", nbuf_Kind_UINT,
		/*list=*/false, /*offset=*/0, 2/*uint16*/);

	/* EnumType */
	msgType = nbuf_Schema_msgTypes(&schema, 2);
	set_msgtype(&msgType, "EnumType", 2, 0, 2);
	set_field(&msgType, 0, "name", nbuf_Kind_STR,
		/*list=*/false, /*idx=*/0, 0);
	set_field(&msgType, 1, "values", nbuf_Kind_PTR,
		/*list=*/true, /*idx=*/1, 1/*EnumDesc*/);

	/* FieldDesc */
	msgType = nbuf_Schema_msgTypes(&schema, 3);
	set_msgtype(&msgType, "FieldDesc", 5, 12, 1);
	set_field(&msgType, 0, "name", nbuf_Kind_STR,
		/*list=*/false, /*idx=*/0, 0);
	set_field(&msgType, 1, "kind", nbuf_Kind_ENUM,
		/*list=*/false, /*offset=*/0, 0/*Kind*/);
	set_field(&msgType, 2, "list", nbuf_Kind_BOOL,
		/*list=*/false, /*offset=*/16, 0);
	set_field(&msgType, 3, "tag0", nbuf_Kind_UINT,
		/*list=*/false, /*offset=*/4, 4/*uint32*/);
	set_field(&msgType, 4, "tag1", nbuf_Kind_UINT,
		/*list=*/false, /*offset=*/8, 4/*uint32*/);

	/* MsgType */
	msgType = nbuf_Schema_msgTypes(&schema, 4);
	set_msgtype(&msgType, "MsgType", 4, 4, 2);
	set_field(&msgType, 0, "name", nbuf_Kind_STR,
		/*list=*/false, /*idx=*/0, 0);
	set_field(&msgType, 1, "fields", nbuf_Kind_PTR,
		/*list=*/true, /*idx=*/1, 3/*FieldDesc*/);
	set_field(&msgType, 2, "ssize", nbuf_Kind_UINT,
		/*list=*/false, /*offset=*/0, 2/*uint16*/);
	set_field(&msgType, 3, "psize", nbuf_Kind_UINT,
		/*list=*/false, /*offset=*/2, 2/*uint16*/);

	/* enumTypes:
	 * Kind */
	nbuf_Schema_init_enumTypes(&schema, 1);
	enumType = nbuf_Schema_enumTypes(&schema, 0);
	set_enum(&enumType, "Kind", 7,
		"BOOL", nbuf_Kind_BOOL,
		"ENUM", nbuf_Kind_ENUM,
		"INT", nbuf_Kind_INT,
		"UINT", nbuf_Kind_UINT,
		"FLOAT", nbuf_Kind_FLOAT,
		"STR", nbuf_Kind_STR,
		"PTR", nbuf_Kind_PTR);
}

int
main() {
	struct nbuf_buffer buf;

	nbuf_init_write(&buf, NULL, 0);
	make_schema(&buf);
	fwrite(buf.base, buf.len, 1, stdout);
	return 0;
}
