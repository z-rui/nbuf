/* binary -> text */

#include "nbuf.h"
#include "nbuf.nb.h"

#include <stdio.h>

extern void load(const char *filename, void **msg, size_t *size);

nbuf_Schema schema;
char *prefix, *upper_prefix;
FILE *fout;

void
print_enum(nbuf_EnumType enumType)
{
	nbuf_EnumDesc enumDesc;
	size_t i, n;

	printf("\nenum %s {\n",
		nbuf_EnumType_name(&enumType, NULL));
	n = nbuf_EnumType_values_size(&enumType);
	for (i = 0; i < n; i++) {
		enumDesc = nbuf_EnumType_values(&enumType, i);
		printf("\t%s = %d\n",
			nbuf_EnumDesc_name(&enumDesc, NULL),
			nbuf_EnumDesc_value(&enumDesc));
	}
	printf("}\n");
}

const char *
typestr(nbuf_Kind kind, uint32_t tag1)
{
	static char buf[20];
	nbuf_EnumType enm;
	nbuf_MsgType msg;

	switch (kind) {
	case nbuf_Kind_BOOL:
		return "bool";
	case nbuf_Kind_INT:
		sprintf(buf, "int%d", tag1*8);
		return buf;
	case nbuf_Kind_UINT:
		sprintf(buf, "uint%d", tag1*8);
		return buf;
	case nbuf_Kind_FLOAT:
		return (tag1 == 4) ? "float" : "double";
	case nbuf_Kind_STR:
		return "string";
	case nbuf_Kind_ENUM:
		enm = nbuf_Schema_enumTypes(&schema, tag1);
		return nbuf_EnumType_name(&enm, NULL);
	case nbuf_Kind_PTR:
		msg = nbuf_Schema_msgTypes(&schema, tag1);
		return nbuf_MsgType_name(&msg, NULL);
	default:
		assert(0 && "unknown kind");
		break;
	}
	return NULL;
}

void
print_msgtype(nbuf_MsgType msgType)
{
	nbuf_FieldDesc field;
	nbuf_Kind kind;
	size_t i, n;

	printf("\nmessage %s {\n",
		nbuf_MsgType_name(&msgType, NULL));
	n = nbuf_MsgType_fields_size(&msgType);
	for (i = 0; i < n; i++) {
		field = nbuf_MsgType_fields(&msgType, i);
		kind = nbuf_FieldDesc_kind(&field);
		printf("\t%s: %s%s  // %u\n",
			nbuf_FieldDesc_name(&field, NULL),
			nbuf_FieldDesc_list(&field) ? "[]" : "",
			typestr(kind, nbuf_FieldDesc_tag1(&field)),
			nbuf_FieldDesc_tag0(&field));
	}
	printf("}  // (%u, %u)\n",
		nbuf_MsgType_ssize(&msgType),
		nbuf_MsgType_psize(&msgType));
}

void
print_schema()
{
	const char *pkgName;
	size_t i, n;

	pkgName = nbuf_Schema_pkgName(&schema, &n);
	if (n > 0)
		printf("package %s\n", pkgName);
	n = nbuf_Schema_enumTypes_size(&schema);
	for (i = 0; i < n; i++) {
		print_enum(nbuf_Schema_enumTypes(&schema, i));
	}
	n = nbuf_Schema_msgTypes_size(&schema);
	for (i = 0; i < n; i++) {
		print_msgtype(nbuf_Schema_msgTypes(&schema, i));
	}
}

int
main(int argc, char *argv[])
{
	struct nbuf_buffer buf = {NULL};
	void *base;
	size_t len;

	load(argv[1], &base, &len);
	nbuf_init_read(&buf, base, len);
	schema = nbuf_get_Schema(&buf);
	fout = stdout;
	print_schema();
	return 0;
}
