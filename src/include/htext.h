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

#ifndef HT_H
#define HT_H

#include "htdatabase.h"
#include "utbox.h"

void htStart(void);
void htStop(void);

/* Hash table routines */
extern uint32 htHashValue;
htHtbl htHtblCreate(void);
void htStartHashKey(void);
#define htHashChar(c) (htHashValue = (htHashValue ^ (char)(c)) * 1103515245 + 12345)
void htHashBool(bool value);
void htHashString(char const *name);
void htHashStringWithoutCase(char const *name);
void htHashUint16(uint16 value);
#define htHashUint32(value) (htHashValue = (htHashValue ^ (uint32)(value)) * 1103515245 + 12345)
void htHashBox(utBox box);
#define htHashInt32(value) htHashUint32(value)
#define htHashSym(sym) htHashUint32(utSym2Index(sym))
htEntry htHtblLookupEntry(htHtbl htbl, bool (*matchEntry)(htEntry entry));
uint32 htHtblLookup(htHtbl htbl, bool (*matchEntry)(htEntry entry));
htEntry htHtblAdd(htHtbl htbl, uint32 data);
void htHtblRemoveEntry(htHtbl htbl, htEntry entry);

/* Symbol table support */
void htHtblAddSym(htHtbl htbl, utSym sym);
utSym htHtblLookupSym(htHtbl htbl, utSym sym);

#endif
