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

#include "db.h"
#include "cirdatabase.h"
#include "cirext.h"

void cirStart(dbDesign design, dbSpiceTargetType targetType);
void cirStop(void);
bool cirBuildDevices(void);

extern cirRoot cirTheRoot;
extern char *cirDeviceStringPtr;

/* Lex, yacc stuff */
extern FILE *cirFile;
extern uint32 cirLineNum;
extern int cirparse();
extern int cirlex();
extern void cirerror(char *message, ...);
extern void cirwarn(char *message, ...);

/* Attribute symbols */
utSym cirGraphicalSym, cirNetSym, cirSpiceTypeSym, cirSpiceTextSym;

