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

#include "db.h"
#include "vrdatabase.h"
#include "vrext.h"

/* Constructors */
extern dbNetlist vrNetlistCreate(dbDesign design, utSym name);

/* I/O */
extern void vrPrint(char *newLineText, char *format, ...);
extern void vrPrintLn(char *newLineText, char *format, ...);
extern char *vrMunge(char *name);
extern char *vrMungeBus(char *name);

/* Globals */
extern dbDesign vrCurrentDesign;
extern FILE *vrFile;
extern uint32 vrFileSize, vrLineNum, vrCharCount, vrLinePos;

/* Lex, Yacc stuff */
extern int vrparse();
extern int vrlex();
extern void vrerror(char *message, ...);
extern char *vrtext;
