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
#include "cir.h"
#include "cirparse.h"

#define YY_NEVER_INTERACTIVE 1
#define YY_INPUT(buf,result,max_size) \
    if((result = cirRead((char *) buf, max_size )) < 0 ) \
        YY_FATAL_ERROR( "input in flex scanner failed" );

static int cirRead(char *buf, int maxSize)
{
    int numChars = 0;
    char c;

    do {
        c = *cirDeviceStringPtr;
        if(c != '\0') {
            cirDeviceStringPtr++;
        }
        if(c != '\0' && c != '\r') {
            *buf++ = c;
            numChars++;
        }
    } while(numChars < maxSize && c != '\0');
    return numChars;
}

int cirwrap(void)
{
    return 1;
}

/*--------------------------------------------------------------------------------------------------
  Provide yyerror function capability.
--------------------------------------------------------------------------------------------------*/
void cirerror (
    char *message,
    ...)
{
    char *buff;
    va_list ap;

    va_start(ap, message);
    buff = utVsprintf(message, ap);
    va_end(ap);
    utError("Line %d, token \"%s\": %s", cirLineNum, cirtext, buff);
}

#ifdef CIR_DEBUG
#define myDebug utDebug
#else
#define myDebug noDebug
#endif
#undef YYLMAX
#define YYLMAX 4096

static void noDebug(char *foo, ...) {}

%}

%%
<INITIAL>"*".*"\n"     { cirLineNum++; myDebug("Comment\n"); }
<INITIAL>[\t ]+        ;
<INITIAL>\'[^']*\'     { char *buf = utCopyString(cirtext) + 1;
                         buf[strlen(buf) - 1] = '\0';
                         cirlval.stringVal = utSymCreate(buf);
                         myDebug("string '%s'\n", utSymGetName(cirlval.stringVal));
                         return STRING; }
<INITIAL>[a-zA-Z0-9\-\+,_]+ { cirlval.identVal = utSymCreate(cirtext);
                         myDebug("ident '%s'\n", cirtext);
                         return IDENT; }
<INITIAL>"\n"          { cirLineNum++;
                         myDebug("Char '\\n'\n");
                         return '\n'; }
<INITIAL>.             { myDebug("Char '%c'\n", cirtext[0]);
                         return cirtext[0]; }
