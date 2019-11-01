/* binary -> text */

#include "nbuf.nb.h"
#include "libnbufc.h"

#include <stdio.h>

static void
print_enum(FILE *fout, nbuf_EnumType enumType)
{
	nbuf_EnumDesc enumDesc;
	size_t i, n;

	fprintf(fout, "\nenum %s {\n",
		nbuf_EnumType_name(&enumType, NULL));
	n = nbuf_EnumType_values_size(&enumType);
	for (i = 0; i < n; i++) {
		enumDesc = nbuf_EnumType_values(&enumType, i);
		fprintf(fout, "\t%s = %d\n",
			nbuf_EnumDesc_name(&enumDesc, NULL),
			nbuf_EnumDesc_value(&enumDesc));
	}
	fprintf(fout, "}\n");
}

static const char *
typestr(nbuf_Schema *schema, nbuf_Kind kind, uint32_t tag1)
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
		enm = nbuf_Schema_enumTypes(schema, tag1);
		return nbuf_EnumType_name(&enm, NULL);
	case nbuf_Kind_PTR:
		msg = nbuf_Schema_msgTypes(schema, tag1);
		return nbuf_MsgType_name(&msg, NULL);
	default:
		assert(0 && "unknown kind");
		break;
	}
	return NULL;
}

static void
print_msgtype(nbuf_Schema *schema, FILE *fout,
	nbuf_MsgType msgType)
{
	nbuf_FieldDesc field;
	nbuf_Kind kind;
	size_t i, n;

	fprintf(fout, "\nmessage %s {\n",
		nbuf_MsgType_name(&msgType, NULL));
	n = nbuf_MsgType_fields_size(&msgType);
	for (i = 0; i < n; i++) {
		field = nbuf_MsgType_fields(&msgType, i);
		kind = nbuf_FieldDesc_kind(&field);
		fprintf(fout, "\t%s: %s%s  // %c%u\n",
			nbuf_FieldDesc_name(&field, NULL),
			nbuf_FieldDesc_list(&field) ? "[]" : "",
			typestr(schema, kind, nbuf_FieldDesc_tag1(&field)),
			(kind == nbuf_Kind_PTR || kind == nbuf_Kind_STR ||
				nbuf_FieldDesc_list(&field)) ? 'p' :
				(kind == nbuf_Kind_BOOL) ? 'b' : 's',
			nbuf_FieldDesc_tag0(&field));
	}
	fprintf(fout, "}  // (%us, %up)\n",
		nbuf_MsgType_ssize(&msgType),
		nbuf_MsgType_psize(&msgType));
}

static void
print_schema(nbuf_Schema *schema, FILE *fout)
{
	const char *pkgName;
	size_t i, n;

	pkgName = nbuf_Schema_pkgName(schema, &n);
	if (n > 0)
		fprintf(fout, "package %s\n", pkgName);
	n = nbuf_Schema_enumTypes_size(schema);
	for (i = 0; i < n; i++)
		print_enum(fout, nbuf_Schema_enumTypes(schema, i));
	n = nbuf_Schema_msgTypes_size(schema);
	for (i = 0; i < n; i++)
		print_msgtype(schema, fout,
			nbuf_Schema_msgTypes(schema, i));
}

void
nbuf_b2t(struct nbuf_buffer *buf, FILE *fout)
{
	nbuf_Schema schema = nbuf_get_Schema(buf);
	print_schema(&schema, fout);
}
