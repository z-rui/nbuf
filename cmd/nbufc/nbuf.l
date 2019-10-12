%{
#include "nbuf.tab.h"

#include <string.h>

%}

%option yylineno

ID	[A-Za-z_][A-Za-z_0-9]*

%%

package		return PACKAGE;
message		return MESSAGE;
enum		return ENUM;

{ID}	{
		yylval.s = strdup(yytext);
		return ID;
	}
[0-9]+	{
		yylval.i = atoi(yytext);
		return INT;
	}
"//".*		|
[ \t\r\n\v\f]+	/* skip */
.		return yytext[0];

%%

int yywrap() { return 1; }
