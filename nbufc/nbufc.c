#include "nbuf.h"
#include "libnbufc.h"
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
static const char *srcname = "-";
static const char *outname = "-";
static struct nbuf_buffer buf;

static void
usage()
{
	printf("usage: %s [-h] [-i<fmt> infile] {-o<fmt> outfile}\n\n",
		progname);
	printf("copyright 2019 zr\n");
	printf("compile a schema file.\n\n");
	printf("options:\n");
	printf("  -i<fmt> infile     input format and filename\n");
	printf("  -o<fmt> outfile    output format and filename\n");
	printf("formats:\n");
	printf("  t            io    textual schema\n");
	printf("  b            io    binary schema\n");
	printf("  h             o    c declarations\n");
	printf("  +             o    c++ declarations\n");
	printf("  c             o    c definitions\n");
}

static const char *
flagarg(char ***pargv)
{
	const char *s;

	s = *++*pargv;
	if (s == NULL)
		die("missing argument for %s\n", *--*pargv);
	return s;
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

static bool input_done = false;
static bool output_done = false;

static bool
do_input(char ifmt)
{
	if (input_done)
		die("cannot specify more than one input");
	yyin = myopen(srcname, "rb");
	switch (ifmt) {
	case 't':
		yyparse();
		nbuf_t2b(&buf);
		input_done = true;
		break;
	case 'b':
		load(&buf);
		input_done = true;
		break;
	}
	return input_done;
}

static bool
do_output(char ofmt)
{
	if (!input_done)
		do_input('t');
	if (!input_done)
		die("input failed...why?");
	yyout = myopen(outname, "wb");
	switch (ofmt) {
	case 't':
		nbuf_b2t(&buf, yyout);
		break;
	case 'b':
		dump(&buf);
		break;
	case 'h':
		nbuf_b2h(&buf, yyout, srcname);
		break;
	case 'H':
		nbuf_b2hh(&buf, yyout, srcname);
		break;
	case 'c':
		nbuf_b2c(&buf, yyout, srcname);
		break;
	default:
		return false;
	}
	output_done = true;
	return true;
}

int
main(int argc, char *argv[])
{
	int rc = 0;
	const char *arg;

	nbuf_init_write(&buf, NULL, 0);
	for (progname = *argv++; (arg = *argv) && arg[0] == '-'; ++argv) {
		switch (arg[1]) {
		case 'i':
			srcname = flagarg(&argv);
			if (!do_input(arg[2]))
				goto invalid_option;
			break;
		case 'o':
			outname = flagarg(&argv);
			if (!do_output(arg[2]))
				goto invalid_option;
			break;
		default:
invalid_option:
			fprintf(stderr, "invalid option %s\n", arg);
			rc = 1;
			/* fallthrough */
		case 'h':
			usage();
			exit(rc);
		}
	}
	if (!output_done)
		do_output('t');

	nbuf_free(&buf);
	fclose(yyin);
	fclose(yyout);

	return 0;
}
