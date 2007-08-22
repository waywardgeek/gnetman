/*
 * Copyright (C) 2004 ViASIC
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
#include "vr.h"
#ifdef GNU
#  include "vrytabgnu.h"
#else
#  include "vrparse.h"
#endif

#define YY_NEVER_INTERACTIVE 1
#define YY_INPUT(buf,result,max_size) \
    if((result = vrRead((char *) buf, max_size )) < 0 ) \
        YY_FATAL_ERROR( "input in flex scanner failed" );

static int vrgetc(void)
{
    if(!(vrCharCount++ & 0xffff)) {
/*        utSetProgress((uint8)((100*(double)vrCharCount)/vrFileSize));*/
    }
    return getc(vrFile);
}

static int vrRead(char *buf, int maxSize)
{
    int numChars = 0;
    char c;

    do {
        c = vrgetc();
        if(c != EOF && c != '\r') {
            *buf++ = c;
            numChars++;
        }
    } while(numChars < maxSize && c != EOF);
    return numChars;
}

static char *mungeString(
    char *string)
{
    uint32 length = strlen(string);
    char *newString = utMakeString(length);
    char *p = newString;

    string++;
    while(*string != '"') {
        if(*string == '\\') {
            string++;
            *p = *string;
            p++;
            string++;
        } else {
            *p = *string;
            p++;
            string++;
        }
    }
    *p = '\0';
    utAssert(strlen(newString) < length);
    return newString;
}

int vrwrap(void)
{
    return 1;
}

#ifdef VR_DEBUG
#define myDebug utDebug
#else
#define myDebug noDebug
#endif
#undef YYLMAX
#define YYLMAX 4096

static uint16 vrCommentDepth;

static void noDebug(char *foo, ...) {}

/*--------------------------------------------------------------------------------------------------
        Purpose : Parse a binary formatted integer.
--------------------------------------------------------------------------------------------------*/
static uint32 parseBinaryInteger (
   const char *vrtext,
        uint8 *length)
{
        uint32 value = 0;

   *length = 0;
        while (*vrtext != 'b') {
           *length = *length*10 + *vrtext - '0';
                vrtext++;
        }
   vrtext++;
/* Note: may have to enhance to use bitfield object instead of uint32 */
   while (*vrtext) {
           value = (value << 1) + *vrtext - '0';
                vrtext++;
        }
        return value;
}

%}

%Start comment

%%
<INITIAL>"//".*\n      { vrLineNum++; } /* Single line comment */
<INITIAL>"`".*\n       { vrLineNum++; } /* Ignore simulator directives */
<INITIAL>"/*"          { myDebug("Start block comment...\n");
                         vrCommentDepth = 1;
                                                                 BEGIN comment; }
<comment>"/*"          { vrCommentDepth++; }
<comment>"*/"          { myDebug("End block comment\n");
                         vrCommentDepth--;
                         if (vrCommentDepth == 0) {
                            BEGIN INITIAL;
                         } }
<comment>.             ;
<comment>"\n"          { vrLineNum++; }

<INITIAL>"module"      { myDebug("KWMODULE\n"); return KWMODULE; }
<INITIAL>"endmodule"   { myDebug("KWENDMODULE\n"); return KWENDMODULE; }
<INITIAL>"input"       { myDebug("KWINPUT\n"); return KWINPUT; }
<INITIAL>"output"      { myDebug("KWOUTPUT\n"); return KWOUTPUT; }
<INITIAL>"inout"       { myDebug("KWINOUT\n"); return KWINOUT; }
<INITIAL>"wire"|"wand"|"tri" { myDebug("KWWIRE\n"); return KWWIRE; }
<INITIAL>"assign"      { myDebug("KWASSIGN\n"); return KWASSIGN; }
<INITIAL>"parameter"   { myDebug("KWPARAMETER\n"); return KWPARAMETER; }
<INITIAL>"defparam"   { myDebug("KWDEFPARAM\n"); return KWDEFPARAM; }
<INITIAL>[0-9]+\'b[0-1]+ { vrlval.constVal.mask = parseBinaryInteger(vrtext,
                         &vrlval.constVal.length);
                         myDebug("CONSTANT %u\n", vrlval.constVal.mask);
                         return CONSTANT; }

<INITIAL>[\t ]+        ;
<INITIAL>\n            { vrLineNum++; }
<INITIAL>[0-9]+        { vrlval.intVal = atol(vrtext);
                         myDebug("INTEGER %u\n", vrlval.intVal);
                         return INTEGER; }
<INITIAL>[0-9]+"."[0-9E\-\+]+        { vrlval.doubleVal = strtod(vrtext,NULL);
                         myDebug("DOUBLE %u\n", vrlval.doubleVal);
                         return DOUBLE; }
<INITIAL>[a-zA-Z_][a-zA-Z0-9_]* { myDebug("IDENT %s\n", vrtext);
                         vrlval.symVal = utSymCreate(vrtext);
                         return IDENT; }
<INITIAL>\\[^\t\n ]*   { myDebug("IDENT %s\n", vrtext);
                         vrlval.symVal = utSymCreate(vrtext + 1);
                         return IDENT; }
<INITIAL>\".*\"        { myDebug("STRING %s\n", vrtext);
                         vrlval.symVal = utSymCreate(mungeString(vrtext));
                         return STRING; }
<INITIAL>.             { myDebug("Char '%c'\n", vrtext[0]);
                         return vrtext[0]; }
