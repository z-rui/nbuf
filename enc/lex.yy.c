#include "textnb.tab.h"

#include "libnbuf.h"

extern YYSTYPE yylval;
int yylineno;

int yylex()
{
	static struct nbuf_lexer l;
	int token;

	if (l.lineno == 0)
		nbuf_lex_init(&l, stdin);
	token = nbuf_lex(&l);
	yylineno = l.lineno;
	switch (token) {
	case nbuf_Token_UNK: return 2;
	case nbuf_Token_ID: yylval.s = l.u.s.base; return ID;
	case nbuf_Token_STR: yylval.s = l.u.s.base; return STRING;
	case nbuf_Token_INT: yylval.i = l.u.i; return INT;
	case nbuf_Token_FLT: yylval.f = l.u.f; return FLOAT;
	case EOF: return 0;
	default: return token;
	}
}
