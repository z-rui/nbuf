%{
#include "textnb.tab.h"

#include <inttypes.h>

static char *finish(struct nbuf_buffer *buf);
static void addchr(struct nbuf_buffer *buf, int ch);
static void addstr(struct nbuf_buffer *buf, const char *s, size_t n);

%}

%option yylineno

id	[A-Za-z_][A-Za-z0-9_]*
d	[0-9]
x	[0-9a-fA-F]
int	[+-]?{d}+
fix	({d}+"."{d}*|"."{d}+)
sci	({fix}|{d}+)[eE]{int}
float	[+-]?({fix}|{sci})

%x S_STR

%%
	struct nbuf_buffer buf = {NULL};

{id}	{
	yylval.s = strdup(yytext);
	return ID;
}
{int}	{
	sscanf(yytext, "%" SCNd64, &yylval.i);
	return INT;
}
{float}	{
	sscanf(yytext, "%lf", &yylval.f);
	return FLOAT;
}
\"	BEGIN(S_STR);

[ \t\r\n\f\v]+	|
("#"|"//").*	/* skip */

.	return yytext[0];

<S_STR>{

\"	{
	BEGIN(INITIAL);
	yylval.s = finish(&buf);
	return STRING;
}
\\["\\]	{
	addchr(&buf, yytext[1]);
}
\\x{x}{x}	{
	int ch = 0;
	sscanf(yytext+2, "%x", &ch);
	addchr(&buf, ch);
}
[^\\"]+	{
	addstr(&buf, yytext, yyleng);
}

}  /* <S_STR> */

%%

int yywrap() { return 1; }

char *finish(struct nbuf_buffer *buf)
{
	char *s = realloc(buf->base, buf->len+1);

	s[buf->len] = '\0';
	buf->base = NULL;
	buf->len = buf->cap = 0;
	return s;
}
void addchr(struct nbuf_buffer *buf, int ch)
{
	nbuf_reserve(buf, 1);
	buf->base[buf->len++] = ch;
}
void addstr(struct nbuf_buffer *buf, const char *s, size_t n)
{
	nbuf_reserve(buf, n);
	memcpy(buf->base + buf->len, s, n);
	buf->len += n;
}
