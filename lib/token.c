#include "libnbuf.h"

#include <stdarg.h>
#include <ctype.h>
#include <stdint.h>
#include <stdio.h>

static void
skipline(struct nbuf_lexer *l)
{
	int ch;

	while ((ch = getc(l->fin)) != EOF && ch != '\n')
		;
	ungetc(ch, l->fin);
}

static void
skiplong(struct nbuf_lexer *l)
{
	int ch;

	ch = getc(l->fin);
	while (ch != EOF) {
		if (ch == '*') {
			ch = getc(l->fin);
			if (ch == '/')
				break;
		} else {
			ch = getc(l->fin);
		}
	}
	if (ch == EOF)
		l->error(l, "comment is not closed at EOF");
}

static bool addchr(struct nbuf_buffer *buf, int ch)
{
	if (!nbuf_reserve(buf, 1))
		return false;
	buf->base[buf->len++] = ch;
	return true;
}

#define NEXT do \
	if (addchr(&buf, ch)) \
		ch = getc(l->fin); \
	else \
		goto nomem; \
while (0)

#define isodigit(ch) ('0' <= (ch) && (ch) <= '7')

static nbuf_Token
scannum(struct nbuf_lexer *l, int ch)
{
	int base = 10;
	bool flt = false;
	struct nbuf_buffer buf;
	nbuf_Token token = nbuf_Token_UNK;

	nbuf_init_write(&buf, NULL, 32);
	if (ch == '-')
		NEXT;
	if (ch == '0') {
		NEXT;
		if (ch == 'x' || ch == 'X') {
			base = 16;
			NEXT;
			while (isxdigit(ch))
				NEXT;
		} else if (isodigit(ch)) {
			base = 8;
			NEXT;
			while (isodigit(ch))
				NEXT;
		}
	} else {
		NEXT;
		while (isdigit(ch))
			NEXT;
	}
	if (base == 10) {
		if (ch == '.') {
			flt = true;
			NEXT;
			while (isdigit(ch))
				NEXT;
		}
		if (ch == 'e' || ch == 'E') {
			flt = true;
			NEXT;
			if (ch == '+' || ch == '-')
				NEXT;
			while (isdigit(ch))
				NEXT;
		}
	}
	addchr(&buf, '\0');
	if (flt) {
		l->u.f = strtod(buf.base, NULL);
		token = nbuf_Token_FLT;
	} else if (buf.base[0] == '-') {
		l->u.i = strtoll(buf.base, NULL, base);
		token = nbuf_Token_INT;
	} else {
		l->u.u = strtoull(buf.base, NULL, base);
		token = nbuf_Token_INT;
	}
	if (isalnum(ch)) {
		l->error(l, "malformed number: %s%c", buf.base, ch);
	}
	ungetc(ch, l->fin);
	nbuf_free(&buf);
	return token;
nomem:
	l->error(l, "out of memory");
	nbuf_free(&buf);
	return nbuf_Token_UNK;
}

static nbuf_Token
scanident(struct nbuf_lexer *l, int ch)
{
	struct nbuf_buffer buf;

	nbuf_init_write(&buf, NULL, 32);
	NEXT;
	while (isalnum(ch) || ch == '_')
		NEXT;
	ungetc(ch, l->fin);
	addchr(&buf, '\0');
	l->u.s = buf;
	return nbuf_Token_ID;
nomem:
	l->error(l, "out of memory");
	nbuf_free(&buf);
	return nbuf_Token_UNK;
}

static int
skipspaces(struct nbuf_lexer *l)
{
	int ch;

	do
		if ((ch = getc(l->fin)) == '\n')
			l->lineno++;
	while (isspace(ch));
	return ch;
}

static nbuf_Token
scanstr(struct nbuf_lexer *l, int ch)
{
	int delim = ch;
	struct nbuf_buffer buf;

	nbuf_init_write(&buf, NULL, 32);
again:
	ch = getc(l->fin);
	while (ch != delim) {
		if (ch == '\\') {
			int n = 1, x = 0;
			ch = getc(l->fin);
			switch (ch) {
			case 'a': ch = '\a'; break;
			case 'b': ch = '\b'; break;
			case 'f': ch = '\f'; break;
			case 'n': ch = '\n'; break;
			case 'r': ch = '\r'; break;
			case 't': ch = '\t'; break;
			case 'v': ch = '\v'; break;
			case '\\': ch = '\\'; break;
			case '\'': ch = '\''; break;
			case '"': ch = '"'; break;
			case 'x': {
				n = 0;
				ch = getc(l->fin);
				while (isxdigit(ch) && ++n <= 2) {
					x = x * 16 + (ch - '0');
					ch = getc(l->fin);
				}
				if (n > 0)
					ch = x;
				break;
			}
			default: {
				n = 0;
				while (isodigit(ch) && ++n <= 3) {
					x = x * 8 + (ch - '0');
					ch = getc(l->fin);
				}
				if (n > 0)
					ch = x;
				break;
			}
			}
			if (n == 0)
				l->error(l, "bad escape sequence");
		} else if (ch == '\n') {
			l->error(l, "newline within string literal");
			l->lineno++;
		} else if (ch == EOF) {
			l->error(l, "string is not closed at EOF");
			break;
		}
		NEXT;
	}
	ch = skipspaces(l);
	if (ch == delim)
		goto again;
	ungetc(ch, l->fin);
	addchr(&buf, '\0');
	l->u.s = buf;
	return nbuf_Token_STR;
nomem:
	l->error(l, "out of memory");
	nbuf_free(&buf);
	return nbuf_Token_UNK;
}

static void
error(struct nbuf_lexer *l, const char *fmt, ...)
{
	va_list argp;

	fprintf(stderr, "%d: ", l->lineno);
	va_start(argp, fmt);
	vfprintf(stderr, fmt, argp);
	va_end(argp);
	fprintf(stderr, "\n");
}

void
nbuf_lex_init(struct nbuf_lexer *l, FILE *fin)
{
	l->fin = fin;
	l->lineno = 1;
	l->error = &error;
}

nbuf_Token
nbuf_lex(struct nbuf_lexer *l)
{
	int ch;

reinput:
	ch = skipspaces(l);

	switch (ch) {
	case '/':
		ch = getc(l->fin);
		if (ch == '*') {
			skiplong(l);
		} else if (ch == '/') {
			skipline(l);
		}
		goto reinput;
	case '#':
		skipline(l);
		goto reinput;
	case '"':
		return scanstr(l, ch);
	case '-':
		ch = getc(l->fin);
		ungetc(ch, l->fin);
		if (isdigit(ch))
			return scannum(l, '-');
		break;
	default:
		if (isdigit(ch))
			return scannum(l, ch);
		else if (isalpha(ch) || ch == '_')
			return scanident(l, ch);
	}
	return ch;
}
