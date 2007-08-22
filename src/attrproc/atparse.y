/*
 * Copyright (C) 2003 ViASIC
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License 
 * along with this program; if not, write to the Free Software 
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111 USA
 */

%{

#include "stdlib.h"
#include "at.h"

char *atExpressionString;
atContext atCurrentContext;
double atCurrentValue;
uint32 atLineNum;
bool atExpressionPassed;

%}

%union {
    double floatVal;
};

%token <floatVal> IDENT FLOAT

%type <floatVal> expression

%left '+' '-'
%left '*' '/'
%left UMINUS

%%

goal: expression
{
    atCurrentValue = $1;
}
;

expression: '(' expression ')'
{
    $$ = $2;
}
| '{' expression '}'
{
    $$ = $2;
}
| expression '+' expression
{
    $$ = $1 + $3;
}
| expression '-' expression
{
    $$ = $1 - $3;
}
| expression '*' expression
{
    $$ = $1 * $3;
}
| expression '/' expression
{
    $$ = $1 / $3;
}
| '-' expression
{
    $$ = -$2;
}
| FLOAT
{
    $$ = $1;
}
| IDENT
{
    $$ = $1;
}
;

%%
