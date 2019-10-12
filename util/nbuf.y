%{
#include "nbuf.tab.h"
#include "lex.yy.h"
#include "t2b.h"

#include <stdio.h>
#include <stdlib.h>

static void yyerror(const char *);

static struct enumType **elink;
static struct msgType **mlink;
static uint16_t nextEnumVal;

%}

%define parse.error verbose

%union {
	char *s;
	int i;
	struct enumDesc **elink;
	struct fldDesc **flink;
}

%token PACKAGE ENUM MESSAGE
%token <s> ID
%token <i> INT

%type <elink> enum_field_list
%type <flink> msg_field_list
%type <i> eq_int_opt
%type <i> list_spec
%type <s> typeid

%%

input
:	package_opt dcl_list
;

package_opt
:	/* empty */
|	PACKAGE ID
	{
		schema.pkgName = $2;
	}
;

dcl_list
:	/* empty */
	{
		elink = &schema.enums;
		mlink = &schema.msgs;
	}
|	dcl_list ENUM ID
	{
		struct enumType *e;
		*elink = e = malloc(sizeof *e);
		e->name = $3;
		e->vals = NULL;
		e->next = NULL;
	}
	'{' enum_field_list '}'
	{
		elink = &(*elink)->next;
	}
|	dcl_list MESSAGE ID
	{
		struct msgType *m;
		*mlink = m = malloc(sizeof *m);
		m->name = $3;
		m->flds = NULL;
		m->next = NULL;
	}
	'{' msg_field_list '}'
	{
		mlink = &(*mlink)->next;
	}
;

enum_field_list
:	/* empty */
	{
		nextEnumVal = 0;
		$$ = &(*elink)->vals;
	}
|	enum_field_list ID eq_int_opt
	{
		struct enumDesc *d;
		*$1 = d = malloc(sizeof *d);
		d->name = $2;
		d->val = $3;
		d->next = NULL;
		$$ = &d->next;
	}
;

eq_int_opt
:	/* empty */
	{
		$$ = nextEnumVal++;
	}
|	'=' INT
	{
		$$ = $2;
		nextEnumVal = $2 + 1;
	}
;

msg_field_list
:	/* empty */
	{
		$$ = &(*mlink)->flds;
	}
|	msg_field_list ID ':' list_spec typeid
	{
		struct fldDesc *d;
		*$1 = d = malloc(sizeof *d);
		d->name = $2;
		d->tname = $5;
		d->list = ($4 == 0);
		d->next = NULL;
		$$ = &d->next;
	}
;

list_spec
:	/* empty */
	{
		$$ = -1;
	}
|	'[' ']'
	{
		$$ = 0;
	}
;

typeid
:	ID
	{
		$$ = $1;
	}
;

%%

void
yyerror(const char *err)
{
	fprintf(stderr, "%d: %s\n", yylineno, err);
	exit(1);
}
