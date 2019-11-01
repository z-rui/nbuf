#include "nbuf.tab.h"

#include "libnbuf.h"

#include <stdio.h>

extern YYSTYPE yylval;
int yylineno;
FILE *yyin, *yyout;

int yylex()
{
	static struct nbuf_lexer l;
	int token;

	if (!yyin) yyin = stdin;
	if (!yyout) yyout = stdout;
	if (l.lineno == 0)
		nbuf_lex_init(&l, yyin);
	token = nbuf_lex(&l);
	yylineno = l.lineno;
	switch (token) {
	case nbuf_Token_STR:
	case nbuf_Token_UNK:
	case nbuf_Token_FLT:
		return 2;
	case nbuf_Token_ID: {
		int tok = ID;
		char *s = l.u.s.base;
		if (strcmp(s, "package") == 0) {
			tok = PACKAGE;
		} else if (strcmp(s, "message") == 0) {
			tok = MESSAGE;
		} else if (strcmp(s, "enum") == 0) {
			tok = ENUM;
		}
		if (tok == ID) {
			yylval.s = s;
		} else {
			free(s);
		}
		return tok;
	}
	case nbuf_Token_INT:
		yylval.i = l.u.i;
		return INT;
	case EOF:
		return 0;
	default:
		return token;
	}
}

