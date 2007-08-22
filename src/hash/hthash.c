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

#include <string.h>
#include <ctype.h>
#include "htext.h"

uint32 htHashValue;

/*--------------------------------------------------------------------------------------------------
  Clear a hash table.
--------------------------------------------------------------------------------------------------*/
static void clearHtbl(
    htHtbl htbl)
{
    uint32 size = 1 << htHtblGetSizeExp(htbl);
    uint32 xEntry;

    for(xEntry = 0; xEntry < size; xEntry++) {
        htHtblSetiEntry(htbl, xEntry, htEntryNull);
    }
}

/*--------------------------------------------------------------------------------------------------
  Build a hash table.
--------------------------------------------------------------------------------------------------*/
htHtbl htHtblCreate(void)
{
    htHtbl htbl = htHtblAlloc();

    htHtblSetSizeExp(htbl, 8); /* htbls are powers of two in size */
    htHtblSetNumEntries(htbl, 0);
    htHtblAllocEntrys(htbl, 1 << htHtblGetSizeExp(htbl));
    clearHtbl(htbl);
    return htbl;
}

/*--------------------------------------------------------------------------------------------------
  Reset the hash key.
--------------------------------------------------------------------------------------------------*/
void htStartHashKey(void)
{
    htHashValue = 0L;
}

/*--------------------------------------------------------------------------------------------------
  Hash a single character.
--------------------------------------------------------------------------------------------------*/
#if 0    /* implemented as a macro for speed */
void htHashChar(
    char c)
{
    htHashValue = (htHashValue ^ c) * 1103515245 + 12345;
}
#endif

/*--------------------------------------------------------------------------------------------------
  Hash a single character.
--------------------------------------------------------------------------------------------------*/
void htHashBool(
    bool value)
{
    value = value ? 1 : 0;
    htHashValue = (htHashValue ^ value) * 1103515245 + 12345;
}

/*--------------------------------------------------------------------------------------------------
  Find a good hash id for a string.
--------------------------------------------------------------------------------------------------*/
void htHashString(
    char const *name)
{
    do {
        htHashValue = (htHashValue ^ *name) * 1103515245 + 12345;
    } while (*name++);
}

/*--------------------------------------------------------------------------------------------------
  Find a good hash id for a string.
--------------------------------------------------------------------------------------------------*/
void htHashStringWithoutCase(
    char const *name)
{
    do {
        htHashValue = (htHashValue ^ tolower(*name)) * 1103515245 + 12345;
    } while (*name++);
}

/*--------------------------------------------------------------------------------------------------
  Find a good hash id for a uint16.
--------------------------------------------------------------------------------------------------*/
void htHashuint16(
    uint16 value)
{
    htHashChar((char)value);
    htHashChar((char)(value >> 8));
}

/*--------------------------------------------------------------------------------------------------
  Find a good hash id for a uint32
--------------------------------------------------------------------------------------------------*/
#if 0    /* implemented as a macro for speed */
void htHashuint32(
    uint32 value)
{
    htHashChar((char)value);
    htHashChar((char)(value >> 8));
    htHashChar((char)(value >> 16));
    htHashChar((char)(value >> 24));
}
#endif

/*--------------------------------------------------------------------------------------------------
  Hash in a box's coordinates.
--------------------------------------------------------------------------------------------------*/
void htHashBox(
    utBox box)
{
    htHashInt32(utBoxGetLeft(box));
    htHashInt32(utBoxGetBottom(box));
    htHashInt32(utBoxGetRight(box));
    htHashInt32(utBoxGetTop(box));
}

/*--------------------------------------------------------------------------------------------------
  Find an entry in the hash table with the given name.
--------------------------------------------------------------------------------------------------*/
htEntry htHtblLookupEntry(
    htHtbl htbl,
    bool (*matchEntry)(htEntry entry))
{
    uint32 mask = (1 << htHtblGetSizeExp(htbl)) - 1;
    htEntry entry = htHtblGetiEntry(htbl, htHashValue & mask);

    while(entry != htEntryNull) {
        if(matchEntry(entry)) {
            return entry;
        }
        entry = htEntryGetEntry(entry);
    }
    return htEntryNull;
}

/*--------------------------------------------------------------------------------------------------
  Find a value in the hash table with the given name.
--------------------------------------------------------------------------------------------------*/
uint32 htHtblLookup(
    htHtbl htbl,
    bool (*matchEntry)(htEntry entry))
{
    htEntry entry = htHtblLookupEntry(htbl, matchEntry);

    if(entry != htEntryNull) {
        return htEntryGetData(entry);
    }
    return UINT32_MAX;
}

/*--------------------------------------------------------------------------------------------------
  Make a new hash table htbl twice as large, and copy from the old one to the new one.
  Note: We've violated the array abstraction here.
--------------------------------------------------------------------------------------------------*/
static void resizeHtbl(
    htHtbl htbl)
{
    uint16 sizeExp = htHtblGetSizeExp(htbl);
    uint32 oldSize = 1 << sizeExp;
    uint32 newSize = 1 << (sizeExp + 1);
    uint32 mask = newSize - 1;
    htEntry *oldEntries = htHtblGetEntrys(htbl);
    htEntry entry, nEntry;
    uint32 xOldHtbl, xNewHtbl;

    htHtblSetSizeExp(htbl, sizeExp + 1);
    htHtblAllocEntrys(htbl, 1 << htHtblGetSizeExp(htbl));
    clearHtbl(htbl);
    for(xOldHtbl = 0; xOldHtbl < oldSize; xOldHtbl++) {
        for(entry = oldEntries[xOldHtbl]; entry != htEntryNull; entry = nEntry) {
            nEntry = htEntryGetEntry(entry);
            xNewHtbl = htEntryGetHashValue(entry) & mask;
            htEntrySetEntry(entry, htHtblGetiEntry(htbl, xNewHtbl));
            htHtblSetiEntry(htbl, xNewHtbl, entry);
        }
    }
    utFree(oldEntries);
}

/*--------------------------------------------------------------------------------------------------
  Add an entry to the htbl.  If the table is near full, build a new one twice as big, delete the
  old one, and return the new one.  Otherwise, return the current hash table.
--------------------------------------------------------------------------------------------------*/
htEntry htHtblAdd(
    htHtbl htbl,
    uint32 data)
{
    uint32 mask;
    uint32 index;
    htEntry entry = htEntryAlloc();

    htEntrySetHashValue(entry, htHashValue);
    htEntrySetData(entry, data);
    if(htHtblGetNumEntries(htbl) > (uint32)(1 << (htHtblGetSizeExp(htbl) - 1))) {
        resizeHtbl(htbl);
    }
    mask = (1 << htHtblGetSizeExp(htbl)) - 1;
    index = htHashValue & mask;
    htEntrySetEntry(entry, htHtblGetiEntry(htbl, index));
    htHtblSetiEntry(htbl, index, entry);
    htHtblSetNumEntries(htbl, htHtblGetNumEntries(htbl) + 1);
    return entry;
}

/*--------------------------------------------------------------------------------------------------
  Delete an entry in the hash table.
--------------------------------------------------------------------------------------------------*/
void htDeleteHtblEntry(
    htHtbl htbl,
    htEntry entry)
{
    uint32 mask = (1 << htHtblGetSizeExp(htbl)) - 1;
    uint32 index = htEntryGetHashValue(entry) & mask;
    htEntry pEntry, nEntry;

    nEntry = htHtblGetiEntry(htbl, index);
    if(nEntry == entry) {
        htHtblSetiEntry(htbl, index, htEntryGetEntry(nEntry));
    } else {            
        do {
            pEntry = nEntry;
            nEntry = htEntryGetEntry(nEntry);
        } while (nEntry != entry);        
        htEntrySetEntry(pEntry, htEntryGetEntry(entry));
    }
    htEntryFree(entry);
    htHtblSetNumEntries(htbl, htHtblGetNumEntries(htbl) - 1);
}

/*--------------------------------------------------------------------------------------------------
  Add a symbol to the hash table.
--------------------------------------------------------------------------------------------------*/
void htHtblAddSym(
    htHtbl htbl,
    utSym sym)
{
    htStartHashKey();
    htHashUint32(utSym2Index(sym));
    htHtblAdd(htbl, utSym2Index(sym));
}

static utSym htSym;
/*--------------------------------------------------------------------------------------------------
  Check that the symbol matches.
--------------------------------------------------------------------------------------------------*/
static bool matchSym(
    htEntry entry)
{
    return htEntryGetData(entry) == utSym2Index(htSym);
}

/*--------------------------------------------------------------------------------------------------
  Find the symbol in the hash table, and return its data, or UINT32_MAX if not found.
--------------------------------------------------------------------------------------------------*/
utSym htHtblLookupSym(
    htHtbl htbl,
    utSym sym)
{
    htStartHashKey();
    htHashUint32(utSym2Index(sym));
    htSym = sym;
    return utIndex2Sym(htHtblLookup(htbl, matchSym));
}

/*--------------------------------------------------------------------------------------------------
  Initialize hash table memory.
--------------------------------------------------------------------------------------------------*/
void htStart(void)
{
    htDatabaseStart();
}

/*--------------------------------------------------------------------------------------------------
  Free hash table memory.
--------------------------------------------------------------------------------------------------*/
void htStop(void)
{
    htDatabaseStop();
}

