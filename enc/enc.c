#include "libnbuf.h"

#include <stdio.h>
#include <string.h>

nbuf_Schema schema;
nbuf_MsgType rootType;
struct nbuf_buffer buf;

extern int yyparse(void);

#define die(fmt, ...) do { \
	fprintf(stderr, fmt"\n", ##__VA_ARGS__); \
	exit(1); \
} while (0)

static void
load_schema(const char *path)
{
	static struct nbuf_buffer buf;

	FILE *f = fopen(path, "rb");
	if (f == NULL) {
		perror("fopen");
		die("open error");
	}
	if (!nbuf_load_file(&buf, f)) {
		perror("nbuf_load_file");
		die("read error");
	}
	fclose(f);
	schema = nbuf_get_Schema(&buf);
}

int main(int argc, char *argv[])
{
	const char *rootTypeName = NULL;
#if YYDEBUG
	yydebug = 0;
#endif
	--argc;
	++argv;
	while (argc && **argv == '-') {
		if (strcmp(*argv, "-t") == 0) {
			if (--argc == 0)
				die("missing argument for -t");
			rootTypeName = *++argv;
		} else {
			die("unknown option %s", *argv);
		}
		--argc;
		++argv;
	}
	if (argc != 1)
		die("need exactly 1 argument, got %d", argc);
	load_schema(argv[0]);
	if (rootTypeName) {
		if (!nbuf_Schema_msgType_by_name(&rootType, &schema, rootTypeName))
			die("no definition of message %s", rootTypeName);
	} else if (nbuf_Schema_msgTypes_size(&schema) > 0) {
		rootType = nbuf_Schema_msgTypes(&schema, 0);
	} else {
		die("no definition of messages");
	}

	nbuf_init_write(&buf, NULL, 0);
	yyparse();
	fwrite(buf.base, 1, buf.len, stdout);
	nbuf_free(&buf);
	return 0;
}
