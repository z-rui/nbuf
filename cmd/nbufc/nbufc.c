#include "nbuf.h"
#include "libnbuf.h"
#include "t2b.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define die(...) do { \
	fprintf(stderr, __VA_ARGS__); \
	exit(1); \
} while (0)

extern FILE *yyin, *yyout;
extern int yyparse();

static const char *progname;
static const char *outname = "-";
static char ifmt = 't', ofmt = 'c';

static void
usage()
{
	printf("usage: %s [-o outfile] [-I fmt] [-O fmt] [-h] infile\n\n",
		progname);
	printf("copyright 2019 zr\n");
	printf("compile a schema file.\n\n");
	printf("options:\n");
	printf("  -o outfile    output filename (default: -)\n");
	printf("  -I fmt        input format (default: t)\n");
	printf("  -O fmt        output format (default: c)\n");
	printf("formats:\n");
	printf("  t[ext]    IO  textual schema\n");
	printf("  b[inary]  IO  binary schema\n");
	printf("  c          O  c header\n");
}

static const char *
flagarg(char ***pargv)
{
	const char *s;

	if ((**pargv)[2])
		return **pargv + 2;
	s = *++*pargv;
	if (s == NULL) {
		fprintf(stderr, "missing argument for %s\n",
			*--*pargv);
		exit(1);
	}
	return s;
}

static void
parseflags(int *pargc, char ***pargv)
{
	char **argv = *pargv;
	const char *arg;
	int rc = 0;

	for (progname = *(*pargv)++;
		(arg = **pargv) && arg[0] == '-';
		++*pargv) {
		switch (arg[1]) {
		case 'o':
			outname = flagarg(pargv);
			break;
		case 'I':
			ifmt = *flagarg(pargv);
			break;
		case 'O':
			ofmt = *flagarg(pargv);
			break;
		case '\0':
			goto out;
		default:
			fprintf(stderr, "invalid option %s\n", arg);
			rc = 1;
			/* fallthrough */
		case 'h':
			usage();
			exit(rc);
		}
	}
out:
	*pargc -= *pargv - argv;
}

static FILE *
myopen(const char *path, const char *mode)
{
	FILE *f;

	if (strcmp(path, "-") == 0)
		f = (mode[0] == 'r') ? stdin
			: (mode[0] == 'w') ? stdout
			: NULL;
	else
		f = fopen(path, mode);
	if (f == NULL)
		die("%s: cannot open file %s\n", progname, path);
	return f;
}

static void
load(struct nbuf_buffer *buf)
{
	for (;;) {
		size_t nread;
		nbuf_reserve(buf, NBUF_ALLOC_INC);
		nread = fread(buf->base + buf->len, 1, NBUF_ALLOC_INC, yyin);
		buf->len += nread;
		if (nread < NBUF_ALLOC_INC)
			break;
	}
	if (ferror(yyin)) {
		perror(progname);
		die("read error\n");
	}
}

static void
dump(struct nbuf_buffer *buf)
{
	fwrite(buf->base, buf->len, 1, yyout);
}

int
main(int argc, char *argv[])
{
	struct nbuf_buffer buf;
	const char *srcname;

	parseflags(&argc, &argv);
	if (argc != 1) {
		fprintf(stderr, "need exactly 1 argument\n");
		usage();
		return 1;
	}
	srcname = *argv;
	yyin = myopen(srcname, "rb");
	yyout = myopen(outname, "wb");

	nbuf_init_write(&buf, NULL, 0);
	switch (ifmt) {
	case 't':
		yyparse();
		nbuf_t2b(&buf);
		break;
	case 'b':
		load(&buf);
		break;
	default:
		die("unknown input format\n");
	}
	switch (ofmt) {
	case 't':
		nbuf_b2t(&buf, yyout);
		break;
	case 'b':
		dump(&buf);
		break;
	case 'c':
		nbuf_b2h(&buf, yyout, srcname);
		break;
	default:
		die("unknown output format\n");
	}
	nbuf_free(&buf);
	fclose(yyin);
	fclose(yyout);

	return 0;
}
