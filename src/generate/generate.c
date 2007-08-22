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

#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <math.h>
#include "db.h"
#include "htext.h"
#include "geext.h"

/* This is the rounding error we allow in computations */
#define GE_ERROR_LIMIT 0.00001

#define geRound(value) ((value) >= 0? ((int32)((value) + 0.5)) : ((int32)((value) - 0.5)))

utSym geRES250Sym, geRES6KSym;
static utSym geCSym, geRSym, geValueSym, geSpiceTypeSym;
static dbNetlist geRes250Netlist, geRes6KNetlist, geCapNetlists[20];
static htHtbl geResTable, geCapTable;

/*--------------------------------------------------------------------------------------------------
  Convert a SPICE string into a value.
--------------------------------------------------------------------------------------------------*/
static double findSpiceValue(
    utSym valueSym)
{
    char *name = utSymGetName(valueSym);
    char *endPtr;
    double value = strtod(name, &endPtr);

    if(endPtr != name && *endPtr != '\0') {
        value *= dbFindScalingFactor(endPtr);
    }
    return value;
}

/*--------------------------------------------------------------------------------------------------
  Create a netlist for the 250 Ohm resistor.
--------------------------------------------------------------------------------------------------*/
static dbNetlist buildRes250Netlist(void)
{
    dbNetlist netlist = dbNetlistCreate(dbCurrentDesign, utSymCreate("RES250"), DB_SUBCIRCUIT,
        utSymNull);
    dbMport aMport = dbMportCreate(netlist, utSymCreate("A"), DB_PAS);
    dbMport bMport = dbMportCreate(netlist, utSymCreate("B"), DB_PAS);
    dbPort aPort, bPort;
    dbNet a, b;

    dbFlagInstCreate(aMport);
    dbFlagInstCreate(bMport);
    aPort = dbMportGetFlagPort(aMport);
    bPort = dbMportGetFlagPort(bMport);
    a = dbNetCreate(netlist, dbMportGetSym(aMport));
    b = dbNetCreate(netlist, dbMportGetSym(bMport));
    dbNetInsertPort(a, aPort);
    dbNetInsertPort(b, bPort);
    dbNetlistSetValue(netlist, geValueSym, utSymCreate("250"));
    return netlist;
}

/*--------------------------------------------------------------------------------------------------
  Create a netlist for the 6K Ohm resistor.
--------------------------------------------------------------------------------------------------*/
static dbNetlist buildRes6KNetlist(void)
{
    dbNetlist netlist = dbNetlistCreate(dbCurrentDesign, utSymCreate("RES6K"), DB_SUBCIRCUIT,
        utSymNull);
    dbMport aMport = dbMportCreate(netlist, utSymCreate("A"), DB_PAS);
    dbMport bMport = dbMportCreate(netlist, utSymCreate("B"), DB_PAS);
    dbPort aPort, bPort;
    dbNet a, b;

    dbFlagInstCreate(aMport);
    dbFlagInstCreate(bMport);
    aPort = dbMportGetFlagPort(aMport);
    bPort = dbMportGetFlagPort(bMport);
    a = dbNetCreate(netlist, dbMportGetSym(aMport));
    b = dbNetCreate(netlist, dbMportGetSym(bMport));
    dbNetInsertPort(a, aPort);
    dbNetInsertPort(b, bPort);
    dbNetlistSetValue(netlist, geValueSym, utSymCreate("6000"));
    return netlist;
}

/*--------------------------------------------------------------------------------------------------
  Add the netlist to the hash table (by value).
--------------------------------------------------------------------------------------------------*/
static void addNetlistToTable(
    htHtbl htbl,
    dbNetlist netlist,
    utSym value)
{
    htStartHashKey();
    htHashUint32(utSym2Index(value));
    htHtblAdd(htbl, dbNetlist2Index(netlist));
}

static utSym geValue;
/*--------------------------------------------------------------------------------------------------
  Determine if this netlist matches.
--------------------------------------------------------------------------------------------------*/
static bool matchNetlist(
    htEntry entry)
{
    dbNetlist netlist = dbIndex2Netlist(htEntryGetData(entry));
    utSym valueSym = dbFindAttrValue(dbNetlistGetAttr(netlist), geValueSym);

    if(valueSym == geValue) {
        return true;
    }
    return false;
}

/*--------------------------------------------------------------------------------------------------
  Find an existing netlist in the hash table with the given value.
--------------------------------------------------------------------------------------------------*/
static dbNetlist findNetlistInTable(
    htHtbl htbl,
    utSym value)
{
    htEntry entry;

    geValue = value;
    htStartHashKey();
    htHashSym(value);
    entry = htHtblLookupEntry(htbl, matchNetlist);
    if(entry == htEntryNull) {
        return dbNetlistNull;
    }
    return dbIndex2Netlist(htEntryGetData(entry));
}

/*--------------------------------------------------------------------------------------------------
  Generate a resistor netlist.
--------------------------------------------------------------------------------------------------*/
static dbNetlist generateResistorNetlist(
    uint32 value)
{
    utSym name = dbDesignCreateUniqueNetlistName(dbCurrentDesign, utSprintf("res%u", value));
    dbNetlist netlist = dbNetlistCreate(dbCurrentDesign, name, DB_SUBCIRCUIT, utSymNull);
    dbMport aMport = dbMportCreate(netlist, utSymCreate("n1"), DB_PAS);
    dbMport bMport = dbMportCreate(netlist, utSymCreate("n2"), DB_PAS);
    dbMport res250A = dbNetlistGetFirstMport(geRes250Netlist);
    dbMport res250B = dbMportGetNextNetlistMport(res250A);
    dbMport res6KA = dbNetlistGetFirstMport(geRes6KNetlist);
    dbMport res6KB = dbMportGetNextNetlistMport(res6KA);
    dbNet a, b = dbNetNull;
    dbInst res;
    dbPort aPort, bPort;
    utSym valueSym = utSymCreateFormatted("%u", value);
    uint32 remainingValue = value;
    uint32 xResistor = 1;

    utIfVerbose(1) {
        utLogMessage("Generating resistor netlist %s of value %u Ohms",  utSymGetName(name), value);
    }
    dbNetlistSetValue(netlist, geValueSym, valueSym);
    dbFlagInstCreate(aMport);
    dbFlagInstCreate(bMport);
    aPort = dbMportGetFlagPort(aMport);
    a = dbNetCreate(netlist, dbMportGetSym(aMport));
    dbNetInsertPort(a, aPort);
    while(remainingValue >= 6000) {
        name = utSymCreateFormatted("resistor%d", xResistor);
        res = dbInstCreate(netlist, name, geRes6KNetlist);
        aPort = dbPortCreate(res, res6KA);
        bPort = dbPortCreate(res, res6KB);
        dbNetInsertPort(a, aPort);
        if(remainingValue > 250) {
            name = utSymCreateFormatted("r%u", xResistor);
        } else {
            name = dbMportGetSym(bMport);
        }
        b = dbNetCreate(netlist, name);
        dbNetInsertPort(b, bPort);
        remainingValue -= 6000;
        xResistor++;
        a = b;
    }
    while(remainingValue >= 250) {
        name = utSymCreateFormatted("resistor%d", xResistor);
        res = dbInstCreate(netlist, name, geRes250Netlist);
        aPort = dbPortCreate(res, res250A);
        bPort = dbPortCreate(res, res250B);
        dbNetInsertPort(a, aPort);
        if(remainingValue > 250) {
            name = utSymCreateFormatted("r%u", xResistor);
        } else {
            name = dbMportGetSym(bMport);
        }
        b = dbNetCreate(netlist, name);
        dbNetInsertPort(b, bPort);
        remainingValue -= 250;
        xResistor++;
        a = b;
    }
    bPort = dbMportGetFlagPort(bMport);
    dbNetInsertPort(b, bPort);
    addNetlistToTable(geResTable, netlist, valueSym);
    return netlist;
}

/*--------------------------------------------------------------------------------------------------
  Generate a resistor.  The hard part is all the glue to replace the sub-netlist.  We want to
  call Confluence for general purpose generation.
--------------------------------------------------------------------------------------------------*/
static bool generateResistor(
    dbInst inst)
{
    utSym valueSym = dbFindAttrValue(dbInstGetAttr(inst), geValueSym);
    dbNetlist resistorNetlist;
    double floatValue = findSpiceValue(valueSym);
    uint32 value;
    bool passed = true;

    if(fabs(250*geRound(floatValue/250.0) - floatValue) > GE_ERROR_LIMIT) {
        utWarning("Resistor value of %s in netlist %s is not a multiple of 250 Ohms",
            dbInstGetUserName(inst), dbNetlistGetName(dbInstGetNetlist(inst)));
        passed = false;
    }
    value = 250*geRound(floatValue/250.0);
    value = utMax(value, 250);
    resistorNetlist = findNetlistInTable(geResTable, utSymCreateFormatted("%u", value));
    if(resistorNetlist == dbNetlistNull) {
        resistorNetlist = generateResistorNetlist(value);
    }
    dbInstReplaceInternalNetlist(inst, resistorNetlist);
    return passed;
}

/*--------------------------------------------------------------------------------------------------
  Generate resistor devices using the unit resistors found on the Triad chip.
--------------------------------------------------------------------------------------------------*/
static bool generateResistors(
    dbNetlist netlist)
{
    dbInst inst;
    bool passed = true;

    dbSafeForeachNetlistExternalInst(netlist, inst) {
        if(!generateResistor(inst)) {
            passed = false;
        }
    } dbEndSafeForeachNetlistExternalInst;
    return passed;
}

/*--------------------------------------------------------------------------------------------------
  Find the name of the new cap netlist, given the netlist index.
--------------------------------------------------------------------------------------------------*/
static utSym findCapNetlistName(
    uint32 xNetlist)
{
    if((xNetlist & 1) == 0) {
        return utSymCreateFormatted("C1R%u", xNetlist >> 1);
    }
    if(xNetlist == 1) {
        return utSymCreateFormatted("C1R05");
    }
    return utSymCreateFormatted("C1R%u", xNetlist*5);
}

/*--------------------------------------------------------------------------------------------------
  Create a netlist for the .1pF capacitor.
--------------------------------------------------------------------------------------------------*/
static dbNetlist buildUnitCapNetlist(
    uint32 xNetlist)
{
    utSym name = findCapNetlistName(xNetlist);
    dbNetlist netlist = dbNetlistCreate(dbCurrentDesign, name, DB_SUBCIRCUIT, utSymNull);
    dbMport aMport = dbMportCreate(netlist, utSymCreate("T"), DB_PAS);
    dbMport bMport = dbMportCreate(netlist, utSymCreate("B"), DB_PAS);
    dbPort aPort, bPort;
    dbNet a, b;

    dbFlagInstCreate(aMport);
    dbFlagInstCreate(bMport);
    aPort = dbMportGetFlagPort(aMport);
    bPort = dbMportGetFlagPort(bMport);
    a = dbNetCreate(netlist, dbMportGetSym(aMport));
    b = dbNetCreate(netlist, dbMportGetSym(bMport));
    dbNetInsertPort(a, aPort);
    dbNetInsertPort(b, bPort);
    dbNetlistSetValue(netlist, geValueSym, utSymCreateFormatted("%.3fpF", 0.1 + xNetlist*0.005));
    return netlist;
}

/*--------------------------------------------------------------------------------------------------
  Create all 20 netlists used in the MSSA.
--------------------------------------------------------------------------------------------------*/
static void buildUnitCapNetlists(void)
{
    dbNetlist netlist;
    uint32 xNetlist;

    for(xNetlist = 0; xNetlist < 20; xNetlist++) {
        netlist = buildUnitCapNetlist(xNetlist);
        geCapNetlists[xNetlist] = netlist;
    }
}

/*--------------------------------------------------------------------------------------------------
  Generate a capacitor netlist.  Value is in nF.
--------------------------------------------------------------------------------------------------*/
static dbNetlist generateCapacitorNetlist(
    uint32 value)
{
    utSym name = dbDesignCreateUniqueNetlistName(dbCurrentDesign, utSprintf("cap%u", value));
    dbNetlist netlist = dbNetlistCreate(dbCurrentDesign, name, DB_SUBCIRCUIT, utSymNull);
    dbNetlist capNetlist = geCapNetlists[0];
    dbMport aMport = dbMportCreate(netlist, utSymCreate("n1"), DB_PAS);
    dbMport bMport = dbMportCreate(netlist, utSymCreate("n2"), DB_PAS);
    dbMport capA = dbNetlistGetFirstMport(capNetlist);
    dbMport capB = dbMportGetNextNetlistMport(capA);
    dbNet a, b;
    dbInst cap;
    dbPort aPort, bPort;
    utSym valueSym = utSymCreateFormatted("%.3fpF", value*0.001);
    uint32 remainingValue = value;
    uint32 xCapacitor = 1;

    utIfVerbose(1) {
        utLogMessage("Generating capacitor netlist %s of value %f pF",  utSymGetName(name),
            0.001*value);
    }
    dbNetlistSetValue(netlist, geValueSym, valueSym);
    dbFlagInstCreate(aMport);
    dbFlagInstCreate(bMport);
    aPort = dbMportGetFlagPort(aMport);
    bPort = dbMportGetFlagPort(bMport);
    a = dbNetCreate(netlist, dbMportGetSym(aMport));
    b = dbNetCreate(netlist, dbMportGetSym(bMport));
    dbNetInsertPort(a, aPort);
    dbNetInsertPort(b, bPort);
    while(remainingValue >= 200) {
        name = utSymCreateFormatted("capacitor%d", xCapacitor);
        cap = dbInstCreate(netlist, name, capNetlist);
        aPort = dbPortCreate(cap, capA);
        bPort = dbPortCreate(cap, capB);
        dbNetInsertPort(a, aPort);
        dbNetInsertPort(b, bPort);
        remainingValue -= 100;
        xCapacitor++;
    }
    if(remainingValue >= 100) {
        name = utSymCreateFormatted("capacitor%d", xCapacitor);
        capNetlist = geCapNetlists[(remainingValue - 100)/5];
        capA = dbNetlistGetFirstMport(capNetlist);
        capB = dbMportGetNextNetlistMport(capA);
        cap = dbInstCreate(netlist, name, capNetlist);
        aPort = dbPortCreate(cap, capA);
        bPort = dbPortCreate(cap, capB);
        dbNetInsertPort(a, aPort);
        dbNetInsertPort(b, bPort);
    }
    addNetlistToTable(geCapTable, netlist, valueSym);
    return netlist;
}

/*--------------------------------------------------------------------------------------------------
  Generate a capacitor.  The hard part is all the glue to replace the sub-netlist.  We want to
  call Confluence for general purpose generation.
--------------------------------------------------------------------------------------------------*/
static bool generateCapacitor(
    dbInst inst)
{
    utSym valueSym = dbFindAttrValue(dbInstGetAttr(inst), geValueSym);
    dbNetlist capacitorNetlist;
    double floatValue = 2.0e14*findSpiceValue(valueSym); /* Multiple of 5nf */
    uint32 value;
    bool passed = true;

    if(fabs(geRound(floatValue) - floatValue) > GE_ERROR_LIMIT) {
        utWarning("Capacitor value of %s in netlist %s is not a multiple of 5nF",
            dbInstGetUserName(inst), dbNetlistGetName(dbInstGetNetlist(inst)));
        passed = false;
    }
    value = 5*geRound(floatValue);
    if(value < 100) {
        utWarning("Capacitor value of %s in netlist %s must be >= 0.1pF", dbInstGetUserName(inst),
            dbNetlistGetName(dbInstGetNetlist(inst)));
        passed = false;
    }
    capacitorNetlist = findNetlistInTable(geCapTable, utSymCreateFormatted("%.3fpF", value*0.001));
    if(capacitorNetlist == dbNetlistNull) {
        capacitorNetlist = generateCapacitorNetlist(value);
    }
    dbInstReplaceInternalNetlist(inst, capacitorNetlist);
    return passed;
}

/*--------------------------------------------------------------------------------------------------
  Generate capacitor devices using the unit capacitors found on the Triad chip.
--------------------------------------------------------------------------------------------------*/
static bool generateCapacitors(
    dbNetlist netlist)
{
    dbInst inst;
    bool passed = true;

    dbSafeForeachNetlistExternalInst(netlist, inst) {
        if(!generateCapacitor(inst)) {
            passed = false;
        }
    } dbEndSafeForeachNetlistExternalInst;
    return passed;
}

/*--------------------------------------------------------------------------------------------------
  Generate resistor and capacitor devices using the unit resistors and capacitors found on the
  Triad chip.
--------------------------------------------------------------------------------------------------*/
bool geGenerateDevices(void)
{
    dbNetlist netlist;
    bool passed = true;
    dbAttr deviceAttrs;
    utSym deviceType;

    utLogMessage("Generating SPICE components");
    geCSym = utSymCreate("c");
    geRSym = utSymCreate("r");
    geValueSym = utSymCreate("value");
    geSpiceTypeSym = utSymCreate("spicetype");
    geRes250Netlist = buildRes250Netlist();
    geRes6KNetlist = buildRes6KNetlist();
    buildUnitCapNetlists();
    geResTable = htHtblCreate();
    geCapTable = htHtblCreate();
    dbForeachDesignNetlist(dbCurrentDesign, netlist) {
        if(dbNetlistGetType(netlist) == DB_DEVICE) {
            deviceAttrs = dbNetlistGetAttr(netlist);
            deviceType = dbFindAttrValue(deviceAttrs, geSpiceTypeSym);
            if(deviceType != utSymNull) {
                deviceType = utSymGetLowerSym(deviceType);
                if(deviceType == geRSym) {
                    if(!generateResistors(netlist)) {
                        passed = false;
                    }
                } else if(deviceType == geCSym) {
                    if(!generateCapacitors(netlist)) {
                        passed = false;
                    }
                }
            }
        }
    } dbEndDesignNetlist;
    htHtblDestroy(geResTable);
    htHtblDestroy(geCapTable);
    return passed;
}
