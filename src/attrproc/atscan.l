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

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "at.h"
#include "atparse.h"

#define YY_NEVER_INTERACTIVE 1
#define YY_INPUT(buf,result,max_size) \
    if((result = atRead((char *) buf, max_size )) < 0 ) \
        YY_FATAL_ERROR( "input in flex scanner failed" );

static int atRead(char *buf, int maxSize)
{
    int numChars = 0;
    char c;

    do {
        c = *atExpressionString;
        if(c != '\0') {
            atExpressionString++;
        }
        if(c != '\0' && c != '\r') {
            *buf++ = c;
            numChars++;
        }
    } while(numChars < maxSize && c != '\0');
    return numChars;
}

int atwrap(void)
{
    return 1;
}

/*--------------------------------------------------------------------------------------------------
  Provide yyerror function capability.
--------------------------------------------------------------------------------------------------*/
void aterror (
    char *message,
    ...)
{
    char *buff;
    va_list ap;

    va_start(ap, message);
    buff = utVsprintf(message, ap);
    va_end(ap);
    utError("Line %d, token \"%s\": %s", atLineNum, attext, buff);
}

/*--------------------------------------------------------------------------------------------------
  Find the value of the identifier in the context.
--------------------------------------------------------------------------------------------------*/
static double evalIdent(
    char *identName)
{
    atAttr attr;

    if(atCurrentContext == atContextNull) {
        atExpressionPassed = false;
        return 0.0;
    }
    attr = atContextFindAttr(atCurrentContext, utSymGetLowerSym(utSymCreate(identName)));
    if(attr == atAttrNull) {
        atExpressionPassed = false;
        return 0.0;
    }
    return atAttrGetValue(attr);
}

#ifdef AT_DEBUG
#define myDebug utDebug
#else
#define myDebug noDebug
#endif
#undef YYLMAX
#define YYLMAX 4096

static void noDebug(char *foo, ...) {}

%}

%%
<INITIAL>[\t ]+        ;
<INITIAL>"\n"          { atLineNum++;
                         myDebug("Char '\\n'\n"); }
<INITIAL>[+-]?[0-9]+(\.[0-9]*)?(e[+-]?[0-9]+)?([afpnumkgt]|meg)?  { char *endPtr;
                         atlval.floatVal = strtod(attext, &endPtr);
                         if(endPtr != NULL) {
                             atlval.floatVal *= dbFindScalingFactor(endPtr);
                         }
                         myDebug("FLOAT %f\n", atlval.floatVal);
                         return FLOAT; }
<INITIAL>[+-]?\.[0-9]+(e[+-]?[0-9]+)?([afpnumkgt]|meg)?  { char *endPtr;
                         atlval.floatVal = strtod(attext, &endPtr);
                         if(endPtr != NULL) {
                             atlval.floatVal *= dbFindScalingFactor(endPtr);
                         }
                         myDebug("FLOAT %f\n", atlval.floatVal);
                         return FLOAT; }
<INITIAL>[a-zA-Z0-9\-\+,_]+ { atlval.floatVal = evalIdent(attext);
                         myDebug("ident '%s'\n", attext);
                         return IDENT; }
<INITIAL>.             { myDebug("Char '%c'\n", attext[0]);
                         return attext[0]; }
