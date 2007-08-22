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

/*--------------------------------------------------------------------------------------------------
  Read schematics into the netlist database.
--------------------------------------------------------------------------------------------------*/
#include "sch.h"

/* The root object */
schRoot schTheRoot;

/* Global utSyms */
utSym schRefdesSym, schPinlabelSym, schPinnumberSym, schPinseqSym, schPintypeSym;
utSym schSourceSym, schNetnameSym, schDeviceSym, schFlagSym;
utSym schPowerSym, schNetSym, schValueSym, schCommentSym;

/*--------------------------------------------------------------------------------------------------
  Build utSyms used in the schematic reader.
--------------------------------------------------------------------------------------------------*/
static void buildSyms(void)
{
    schRefdesSym = utSymCreate("refdes");
    schPinlabelSym = utSymCreate("pinlabel");
    schPinnumberSym = utSymCreate("pinnumber");
    schPinseqSym = utSymCreate("pinseq");
    schPintypeSym = utSymCreate("pintype");
    schSourceSym = utSymCreate("source");
    schNetnameSym = utSymCreate("netname");
    schDeviceSym = utSymCreate("device");
    schFlagSym = utSymCreate("FLAG");
    schPowerSym = utSymCreate("POWER");
    schNetSym = utSymCreate("net");
    schValueSym = utSymCreate("value");
    schCommentSym = utSymCreate("comment");
}

/*--------------------------------------------------------------------------------------------------
  Allocate memory used in the schematic netlist reader.
--------------------------------------------------------------------------------------------------*/
void schStart(void)
{
    schDatabaseStart();
    schTheRoot = schRootAlloc();
    buildSyms();
    schStartReader();
    schStartSchem();
    schGensymStart();
}

/*--------------------------------------------------------------------------------------------------
  Free memory used in the schematic netlist reader.
--------------------------------------------------------------------------------------------------*/
void schStop(void)
{
    schGensymStop();
    schStopSchem();
    schStopReader();
    schRootDestroy(schTheRoot);
    schDatabaseStop();
}

/*--------------------------------------------------------------------------------------------------
  Read schematics into the netlist database.
--------------------------------------------------------------------------------------------------*/
dbDesign schReadSchematic(
    char *designName,
    char *fileName,
    dbDesign libDesign)
{
    dbDesign design = dbDesignCreate(utSymCreate(designName), libDesign);
    schSchem schem;
    schSymbol symbol;

    utLogMessage("Reading schematic file %s", fileName);
    schStart();
    schem = schReadSchem(fileName, true);
    if(schem == schSchemNull) {
        return dbDesignNull;
    }
    schSchemCreateDefaultSymbol(schem);
    schBuildNetlists(design, libDesign, schem);
    symbol = schSchemGetSymbol(schem);
    dbDesignSetRootNetlist(design, schSymbolGetNetlist(symbol));
    schStop();
    return design;
}
