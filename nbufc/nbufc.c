#include "libnbuf.h"
#include "libnbufc.h"
#include "t2b.h"
#include "nbuf.tab.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
# include <fcntl.h>
# include <io.h>
#endif

#define die(...) do { \
	fprintf(stderr, __VA_ARGS__); \
	exit(1); \
} while (0)

extern FILE *yyin;
static const char *progname;

static void
usage()
{
	printf("usage: %s cmd ...\n\n", progname);
	printf("copyright 2019 zr\n");
	printf("nbuf compiler.\n\n");
	printf("command        input                output\n");
	printf("  a <s>        binary schema        text schema\n");
	printf("  b <s>        text schema          binary schema\n");
	printf("  c <s>        binary schema        c definitions\n");
	printf("  d <s> [m]    binary message       text message\n");
	printf("  e <s> [m]    text message         binary message\n");
	printf("  h <s>        binary schema        c declarations\n");
	printf("  H <s>        binary schema        c++ declarations\n");
	printf("\narguments\n");
	printf("  <s>          schema file\n");
	printf("  [m]          message type of input, default to the first\n");
	printf("               message type in the schema.\n");
	printf("\nmessages will be read from stdin; ");
	printf("all output is written to stdout.\n");
}

static void
load_schema(struct nbuf_buffer *buf, const char *path, int mode)
{
	FILE *fin = fopen(path, "rb");
	if (fin == NULL) {
		perror("fopen");
		die("open %s failed", path);
	}
	if (mode == 'b') {
		yyin = fin;
		yyparse();
		yyin = NULL;
		nbuf_t2b(buf);
	} else {
		nbuf_load_file(buf, fin);
	}
	fclose(fin);
}

static void
outbin(struct nbuf_buffer *buf)
{
#ifdef _WIN32
	_setmode(_fileno(stdout), _O_BINARY);
#endif
	fwrite(buf->base, 1, buf->len, stdout);
}

static void
enc_dec(struct nbuf_buffer *schema_buf, int mode, const char *rootTypeName)
{
	struct nbuf_buffer obj_buf;
	nbuf_Schema schema = nbuf_get_Schema(schema_buf);
	nbuf_MsgType rootType;

	if (rootTypeName) {
		if (!nbuf_Schema_msgType_by_name(&rootType, schema, rootTypeName))
			die("no message named \"%s\" defined in schema",
				rootTypeName);
	} else if (nbuf_Schema_msgTypes_size(schema) > 0) {
		rootType = nbuf_Schema_msgTypes(schema, 0);
	} else {
		die("no message defined in schema");
	}
	if (mode == 'd') {
		struct nbuf_obj o = {&obj_buf};
		nbuf_init_read(&obj_buf, NULL, 0);
#ifdef _WIN32
		_setmode(_fileno(stdin), _O_BINARY);
#endif
		nbuf_load_file(&obj_buf, stdin);
		nbuf_obj_init(&o, 0);
		nbuf_print(&o, stdout, 2, schema, &rootType);
	} else {
		struct nbuf_lexer l;
		nbuf_lex_init(&l, stdin);
		nbuf_init_write(&obj_buf, NULL, 0);
		nbuf_parse(&obj_buf, &l, schema, rootType);
		outbin(&obj_buf);
	}
	nbuf_free(&obj_buf);
}

int
main(int argc, char *argv[])
{
	static struct nbuf_buffer buf;

#if YYDEBUG
	yydebug = 1;
#endif
	nbuf_init_write(&buf, NULL, 0);
	progname = argv[0];
	if (argc < 3) {
		fprintf(stderr, "expect at least 2 arguments.\n");
		usage();
		return 1;
	}
	load_schema(&buf, argv[2], argv[1][0]);
	switch (argv[1][0]) {
	case 'a':
		nbuf_b2t(&buf, stdout);
		break;
	case 'b':
		outbin(&buf);
		break;
	case 'c':
		nbuf_b2c(&buf, stdout, argv[2]);
		break;
	case 'd':
	case 'e':
		enc_dec(&buf, argv[1][0], argv[3]);
		break;
	case 'h':
		nbuf_b2h(&buf, stdout, argv[2]);
		break;
	case 'H':
		nbuf_b2hh(&buf, stdout, argv[2]);
		break;
	}
	nbuf_free(&buf);
	return 0;
}
