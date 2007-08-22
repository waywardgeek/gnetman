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
  Various short-cuts and misc. routines.
--------------------------------------------------------------------------------------------------*/
#include <stdlib.h>
#include <ctype.h>
#include "db.h"

/* Default mport type names */
utSym dbInSym, dbOutSym, dbIoSym, dbOcSym, dbOeSym, dbPasSym, dbTpSym, dbTriSym, dbClkSym, dbPwrSym;
/* Default direction names */
utSym dbInputSym, dbOutputSym, dbInoutSym;
/* Some mport gate area annotation names */
utSym dbMSym, dbGSym, dbLSym, dbWSym, dbSpiceTypeSym;

/*--------------------------------------------------------------------------------------------------
  Build utSyms used in this module.
--------------------------------------------------------------------------------------------------*/
void dbShortStart(void)
{
    dbInSym = utSymCreate("in");
    dbOutSym = utSymCreate("out");
    dbIoSym = utSymCreate("io");
    dbOcSym = utSymCreate("oc");
    dbOeSym = utSymCreate("oe");
    dbPasSym = utSymCreate("pas");
    dbTpSym = utSymCreate("tp");
    dbTriSym = utSymCreate("tri");
    dbClkSym = utSymCreate("clk");
    dbPwrSym = utSymCreate("pwr");
    dbInputSym = utSymCreate("input");
    dbOutputSym = utSymCreate("output");
    dbInoutSym = utSymCreate("inout");
    dbMSym = utSymCreate("m");
    dbGSym = utSymCreate("g");
    dbLSym = utSymCreate("l");
    dbWSym = utSymCreate("w");
    dbSpiceTypeSym = utSymCreate("spicetype");
}

/*--------------------------------------------------------------------------------------------------
  Free memory used in the short-cut module.
--------------------------------------------------------------------------------------------------*/
void dbShortStop(void)
{
    /* Nothing to do yet */
}

/*--------------------------------------------------------------------------------------------------
  Invert the sense of a port type so that inputs become outputs, etc.
--------------------------------------------------------------------------------------------------*/
dbMportType dbInvertMportType(
    dbMportType type)
{
    if(type == DB_IN) {
        return DB_OUT;
    }
    if(type == DB_OUT) {
        return DB_IN;
    }
    return type;
}

/*--------------------------------------------------------------------------------------------------
  Clear the visited flag on each netlist in the design.
--------------------------------------------------------------------------------------------------*/
void dbDesignClearNetlistVisitedFlags(
   dbDesign design)
{
    dbNetlist netlist;

    dbForeachDesignNetlist(design, netlist) {
        dbNetlistSetVisited(netlist, false);
    } dbEndDesignNetlist;
}

/*--------------------------------------------------------------------------------------------------
  Clear the visited flag on each mport in the netlist.
--------------------------------------------------------------------------------------------------*/
void dbNetlistClearMportVisitedFlags(
   dbNetlist netlist)
{
    dbMport mport;

    dbForeachNetlistMport(netlist, mport) {
        dbMportSetVisited(mport, false);
    } dbEndNetlistMport;
}
    
/*--------------------------------------------------------------------------------------------------
  Clear the visited flag on each net in the netlist.
--------------------------------------------------------------------------------------------------*/
void dbNetlistClearNetVisitedFlags(
   dbNetlist netlist)
{
    dbNet net;

    dbForeachNetlistNet(netlist, net) {
        dbNetSetVisited(net, false);
    } dbEndNetlistNet;
}
    
/*--------------------------------------------------------------------------------------------------
  Visit this netlist's children, then this one.
--------------------------------------------------------------------------------------------------*/
static void visitNetlists(
    dbNetlist netlist,
    void (*func)(dbNetlist))
{
    dbDesign design = dbNetlistGetDesign(netlist);
    dbNetlist internalNetlist;
    dbInst inst;

    dbNetlistSetVisited(netlist, true);
    dbForeachNetlistInst(netlist, inst) {
        internalNetlist = dbInstGetInternalNetlist(inst);
        if(!dbNetlistVisited(internalNetlist) && dbNetlistGetDesign(internalNetlist) == design) {
            visitNetlists(internalNetlist, func);
        }
    } dbEndNetlistInst;
    func(netlist);
}

/*--------------------------------------------------------------------------------------------------
  Visit each netlist used in the design once.  Visit child netlists before parents.
--------------------------------------------------------------------------------------------------*/
void dbDesignVisitNetlists(
    dbDesign design,
    void (*func)(dbNetlist))
{
    dbDesignClearNetlistVisitedFlags(design);
    visitNetlists(dbDesignGetRootNetlist(design), func);
}

/*--------------------------------------------------------------------------------------------------
  Dummy callback for visit netlist routine so we can mark the used ones.
--------------------------------------------------------------------------------------------------*/
static void doNothing(
    dbNetlist netlist)
{
}

/*--------------------------------------------------------------------------------------------------
  Set the visited flags on all used netlists in the design.
--------------------------------------------------------------------------------------------------*/
void dbDesignMarkUsedNetlists(
    dbDesign design)
{
    dbDesignVisitNetlists(design, doNothing);
}

/*--------------------------------------------------------------------------------------------------
  Just return the name of the inst.
--------------------------------------------------------------------------------------------------*/
char *dbInstGetUserName(
    dbInst inst)
{
    dbMbus mbus;
    dbMport mport;

    if(dbNetlistGetType(dbInstGetInternalNetlist(inst)) == DB_FLAG) {
        mbus = dbInstGetExternalMbus(inst);
        if(mbus != dbMbusNull) {
            return dbMbusGetName(mbus);
        }
        mport = dbPortGetMport(dbInstGetFirstPort(inst));
        utAssert(mport != dbMportNull);
        return dbMportGetName(mport);
    }
    return utSymGetName(dbInstGetSym(inst));
}

/*--------------------------------------------------------------------------------------------------
  Return the port name in the format <inst name>.<mport name>
--------------------------------------------------------------------------------------------------*/
char *dbPortGetName(
    dbPort port)
{
    dbInst inst = dbPortGetInst(port);
    dbMport mport = dbPortGetMport(port);

    return utSprintf("%s.%s", dbInstGetUserName(inst), dbMportGetName(mport));
}

/*--------------------------------------------------------------------------------------------------
  Create a unique net name in the schematic.  Make sure it does not collide with a global.
--------------------------------------------------------------------------------------------------*/
utSym dbNetlistCreateUniqueNetName(
    dbNetlist netlist,
    char *name)
{
    dbDesign design = dbNetlistGetDesign(netlist);
    utSym sym = utSymCreate(name);
    uint32 x = 0;

    do {
        if(dbNetlistFindNet(netlist, sym) == dbNetNull) {
            return sym;
        }
        do {
            sym = utSymCreateFormatted("%s%u", name, x);
            x++;
        } while(dbNetlistFindNet(netlist, sym) != dbNetNull);
    } while(dbDesignFindGlobal(design, sym) != dbGlobalNull);
    return sym;
}

/*--------------------------------------------------------------------------------------------------
  Create a unique inst name in the schematic.
--------------------------------------------------------------------------------------------------*/
utSym dbNetlistCreateUniqueInstName(
    dbNetlist netlist,
    char *name)
{
    utSym sym = utSymCreate(name);
    uint32 x = 0;

    if(dbNetlistFindInst(netlist, sym) == dbInstNull) {
        return sym;
    }
    do {
        sym = utSymCreateFormatted("%s%u", name, x);
        x++;
    } while(dbNetlistFindInst(netlist, sym) != dbInstNull);
    return sym;
}

/*--------------------------------------------------------------------------------------------------
  Create a unique mport name in the schematic.
--------------------------------------------------------------------------------------------------*/
utSym dbNetlistCreateUniqueMportName(
    dbNetlist netlist,
    char *name)
{
    utSym sym = utSymCreate(name);
    uint32 x = 0;

    if(dbNetlistFindMport(netlist, sym) == dbMportNull) {
        return sym;
    }
    do {
        sym = utSymCreateFormatted("%s%u", name, x);
        x++;
    } while(dbNetlistFindMport(netlist, sym) != dbMportNull);
    return sym;
}

/*--------------------------------------------------------------------------------------------------
  Create a unique inst name in the schematic.
--------------------------------------------------------------------------------------------------*/
utSym dbDesignCreateUniqueNetlistName(
    dbDesign design,
    char *name)
{
    utSym sym = utSymCreate(name);
    uint32 x = 0;

    if(dbDesignFindNetlist(design, sym) == dbNetlistNull) {
        return sym;
    }
    do {
        sym = utSymCreateFormatted("%s%u", name, x);
        x++;
    } while(dbDesignFindNetlist(design, sym) != dbNetlistNull);
    return sym;
}

/*--------------------------------------------------------------------------------------------------
  Create a unique global net name in the schematic.
--------------------------------------------------------------------------------------------------*/
utSym dbDesignCreateUniqueGlobalName(
    dbDesign design,
    char *name)
{
    utSym sym = utSymCreate(name);
    uint32 x = 0;

    if(dbDesignFindGlobal(design, sym) == dbGlobalNull) {
        return sym;
    }
    do {
        sym = utSymCreateFormatted("%s%u", name, x);
        x++;
    } while(dbDesignFindGlobal(design, sym) != dbGlobalNull);
    return sym;
}

/*--------------------------------------------------------------------------------------------------
  Find the pin type from it's symbol.
--------------------------------------------------------------------------------------------------*/
dbMportType dbFindPinTypeFromSym(
    utSym pinType)
{
    if(pinType == dbInSym) {
        return DB_IN;
    }
    if(pinType == dbOutSym) {
        return DB_OUT;
    }
    if(pinType == dbIoSym) {
        return DB_IO;
    }
    if(pinType == dbOcSym) {
        return DB_OC;
    }
    if(pinType == dbOeSym) {
        return DB_OE;
    }
    if(pinType == dbPasSym) {
        return DB_PAS;
    }
    if(pinType == dbTpSym) {
        return DB_TP;
    }
    if(pinType == dbTriSym) {
        return DB_TRI;
    }
    if(pinType == dbClkSym) {
        return DB_CLK;
    }
    if(pinType == dbPwrSym) {
        return DB_PWR;
    }
    utWarning("Unknown pin type %s", utSymGetName(pinType));
    return DB_IN;
}

/*--------------------------------------------------------------------------------------------------
  Find the symbol for the pin type.
--------------------------------------------------------------------------------------------------*/
utSym dbFindPinTypeSym(
    dbMportType type)
{
    switch(type) {
    case DB_IN: return dbInSym;
    case DB_OUT: return dbOutSym;
    case DB_IO: return dbIoSym;
    case DB_OC: return dbOcSym;
    case DB_OE: return dbOeSym;
    case DB_PAS: return dbPasSym;
    case DB_TP: return dbTpSym;
    case DB_TRI: return dbTriSym;
    case DB_CLK: return dbClkSym;
    case DB_PWR: return dbPwrSym;
    default:
        utExit("dbFindPinTypeSym: Unknown pin type");
    }
    return utSymNull; /* Dummy return */
}

/*--------------------------------------------------------------------------------------------------
  Find the direction for the mport type.
--------------------------------------------------------------------------------------------------*/
dbDirection dbFindMportTypeDirection(
    dbMportType type)
{
    switch(type) {
    case DB_IN:
        return DB_INPUT;
    case DB_OUT:
    case DB_OC:
    case DB_OE:
    case DB_TRI:
    case DB_CLK:
        return DB_OUTPUT;
    case DB_PWR:
    case DB_IO:
    case DB_PAS:
    case DB_TP:
        return DB_INOUT;
    default:
        utExit("dbFindPinTypeSym: Unknown pin type");
    }
    return DB_INPUT; /* Dummy return */
}

/*--------------------------------------------------------------------------------------------------
  Find the symbol for the direction type.
--------------------------------------------------------------------------------------------------*/
utSym dbFindDirectionSym(
    dbDirection direction)
{
    switch(direction) {
    case DB_INPUT: return dbInputSym;
    case DB_OUTPUT: return dbOutputSym;
    case DB_INOUT: return dbInoutSym;
    default:
        utExit("Unknown direction");
    }
    return utSymNull; /* Dummy return */
}

/*--------------------------------------------------------------------------------------------------
  Find the pin type from it's symbol.
--------------------------------------------------------------------------------------------------*/
dbDirection dbFindDirectionFromSym(
    utSym directionSym)
{
    if(directionSym == dbInputSym) {
        return DB_INPUT;
    }
    if(directionSym == dbOutputSym) {
        return DB_OUTPUT;
    }
    if(directionSym == dbInoutSym) {
        return DB_INOUT;
    }
    utWarning("Unknown direcetion %s", utSymGetName(directionSym));
    return DB_INOUT;
}

/*--------------------------------------------------------------------------------------------------
  Determine if a zero or one cell drives this net.
--------------------------------------------------------------------------------------------------*/
bool dbNetIsZeroOrOne(
    dbNet net)
{
    dbDesign design;
    dbGlobal global = dbNetGetGlobal(net);
    dbGlobal one, zero;

    if(global == dbGlobalNull) {
        return false;
    }
    design = dbNetlistGetDesign(dbNetGetNetlist(net));
    one = dbDesignGetOneGlobal(design);
    zero = dbDesignGetZeroGlobal(design);
    return global == one || global == zero;
}

/*--------------------------------------------------------------------------------------------------
  Determine if the net is zero.
--------------------------------------------------------------------------------------------------*/
bool dbNetIsZero(
    dbNet net)
{
    dbDesign design;
    dbGlobal global = dbNetGetGlobal(net);
    dbGlobal zero;

    if(global == dbGlobalNull) {
        return false;
    }
    design = dbNetlistGetDesign(dbNetGetNetlist(net));
    zero = dbDesignGetZeroGlobal(design);
    return global == zero;
}

/*--------------------------------------------------------------------------------------------------
  Determine if the net is one.
--------------------------------------------------------------------------------------------------*/
bool dbNetIsOne(
    dbNet net)
{
    dbDesign design;
    dbGlobal global = dbNetGetGlobal(net);
    dbGlobal one;

    if(global == dbGlobalNull) {
        return false;
    }
    design = dbNetlistGetDesign(dbNetGetNetlist(net));
    one = dbDesignGetOneGlobal(design);
    return global == one;
}

/*--------------------------------------------------------------------------------------------------
  Set the value on the instance.
--------------------------------------------------------------------------------------------------*/
void dbInstSetValue(
    dbInst inst,
    utSym name,
    utSym value)
{
    dbAttr attr = dbFindAttr(dbInstGetAttr(inst), name);

    if(attr == dbAttrNull) {
        attr = dbAttrCreate(name, value);
        dbAttrSetNextAttr(attr, dbInstGetAttr(inst));
        dbInstSetAttr(inst, attr);
    } else {
        dbAttrSetValue(attr, value);
    }
}

/*--------------------------------------------------------------------------------------------------
  Find the attribute value on the inst.
--------------------------------------------------------------------------------------------------*/
utSym dbInstGetValue(
    dbInst inst,
    utSym name)
{
    return dbFindAttrValue(dbInstGetAttr(inst), name);
}

/*--------------------------------------------------------------------------------------------------
  Set the value on the net.
--------------------------------------------------------------------------------------------------*/
void dbNetSetValue(
    dbNet net,
    utSym name,
    utSym value)
{
    dbAttr attr = dbFindAttr(dbNetGetAttr(net), name);

    if(attr == dbAttrNull) {
        attr = dbAttrCreate(name, value);
        dbAttrSetNextAttr(attr, dbNetGetAttr(net));
        dbNetSetAttr(net, attr);
    } else {
        dbAttrSetValue(attr, value);
    }
}

/*--------------------------------------------------------------------------------------------------
  Find the attribute value on the net.
--------------------------------------------------------------------------------------------------*/
utSym dbNetGetValue(
    dbNet net,
    utSym name)
{
    return dbFindAttrValue(dbNetGetAttr(net), name);
}

/*--------------------------------------------------------------------------------------------------
  Set the value on the netlist.
--------------------------------------------------------------------------------------------------*/
void dbNetlistSetValue(
    dbNetlist netlist,
    utSym name,
    utSym value)
{
    dbAttr attr = dbFindAttr(dbNetlistGetAttr(netlist), name);

    if(attr == dbAttrNull) {
        attr = dbAttrCreate(name, value);
        dbAttrSetNextAttr(attr, dbNetlistGetAttr(netlist));
        dbNetlistSetAttr(netlist, attr);
    } else {
        dbAttrSetValue(attr, value);
    }
}

/*--------------------------------------------------------------------------------------------------
  Find the attribute value on the netlist.
--------------------------------------------------------------------------------------------------*/
utSym dbNetlistGetValue(
    dbNetlist netlist,
    utSym name)
{
    return dbFindAttrValue(dbNetlistGetAttr(netlist), name);
}

/*--------------------------------------------------------------------------------------------------
  Set the value on the design.
--------------------------------------------------------------------------------------------------*/
void dbDesignSetValue(
    dbDesign design,
    utSym name,
    utSym value)
{
    dbAttr attr = dbFindAttr(dbDesignGetAttr(design), name);

    if(attr == dbAttrNull) {
        attr = dbAttrCreate(name, value);
        dbAttrSetNextAttr(attr, dbDesignGetAttr(design));
        dbDesignSetAttr(design, attr);
    } else {
        dbAttrSetValue(attr, value);
    }
}

/*--------------------------------------------------------------------------------------------------
  Find the attribute value on the design.
--------------------------------------------------------------------------------------------------*/
utSym dbDesignGetValue(
    dbDesign design,
    utSym name)
{
    return dbFindAttrValue(dbDesignGetAttr(design), name);
}

/*--------------------------------------------------------------------------------------------------
  Set the value on the mport.
--------------------------------------------------------------------------------------------------*/
void dbMportSetValue(
    dbMport mport,
    utSym name,
    utSym value)
{
    dbAttr attr = dbFindAttr(dbMportGetAttr(mport), name);

    if(attr == dbAttrNull) {
        attr = dbAttrCreate(name, value);
        dbAttrSetNextAttr(attr, dbMportGetAttr(mport));
        dbMportSetAttr(mport, attr);
    } else {
        dbAttrSetValue(attr, value);
    }
}

/*--------------------------------------------------------------------------------------------------
  Find the attribute value on the mport.
--------------------------------------------------------------------------------------------------*/
utSym dbMportGetValue(
    dbMport mport,
    utSym name)
{
    return dbFindAttrValue(dbMportGetAttr(mport), name);
}

/*--------------------------------------------------------------------------------------------------
  Index a net in a bus, given the bit.
--------------------------------------------------------------------------------------------------*/
dbNet dbBusIndexNet(
    dbBus bus,
    uint32 bit)
{
    uint32 left = dbBusGetLeft(bus);
    uint32 right = dbBusGetRight(bus);

    if(left < right) {
        return dbBusGetiNet(bus, bit - left);
    }
    return dbBusGetiNet(bus, (right - left) - (bit - right));
}

/*--------------------------------------------------------------------------------------------------
  Index an mport in an mbus, given the bit.
--------------------------------------------------------------------------------------------------*/
dbMport dbMbusIndexMport(
    dbMbus mbus,
    uint32 bit)
{
    uint32 left = dbMbusGetLeft(mbus);
    uint32 right = dbMbusGetRight(mbus);

    if(left < right) {
        return dbMbusGetiMport(mbus, bit - left);
    }
    return dbMbusGetiMport(mbus, (right - left) - (bit - right));
}

/*--------------------------------------------------------------------------------------------------
  Find a SPICE scaling factor.
--------------------------------------------------------------------------------------------------*/
double dbFindScalingFactor(
    char *string)
{
    if(!strncasecmp(string, "meg", 3)) {
        return 1.0e6;
    }
    switch(toupper(*string)) {
    case 'A': return 1.0e-18;
    case 'F': return 1.0e-15;
    case 'P': return 1.0e-12;
    case 'N': return 1.0e-9;
    case 'U': return 1.0e-6;
    case 'M': return 1.0e-3;
    case 'K': return 1.0e3;
    case 'G': return 1.0e9;
    case 'T': return 1.0e12;
    default:
        /* We just ignore unrecognized scaling characters */
        ;
    }
    return 1.0;
}

/*--------------------------------------------------------------------------------------------------
  Return the name of the netlist type.
--------------------------------------------------------------------------------------------------*/
char *dbFindNetlistTypeName(
    dbNetlistType type)
{
    switch(type) {
    case DB_JOIN: return "JOIN";
    case DB_SUBCIRCUIT: return "SUBCIRCUIT";
    case DB_DEVICE: return "DEVICE";
    case DB_POWER: return "POWER";
    case DB_FLAG: return "FLAG";
    default:
        utExit("Unknown netlist type");
    }
    return NULL; /* Dummy return */
}

/*--------------------------------------------------------------------------------------------------
  Determine if the instance is purely graphical, from the attribute.
--------------------------------------------------------------------------------------------------*/
bool dbInstIsGraphical(
    dbInst inst)
{
    dbNetlist internalNetlist = dbInstGetInternalNetlist(inst);
    dbAttr instAttrs = dbNetlistGetAttr(internalNetlist);
 
    if(dbFindAttrNoCase(instAttrs, dbGraphicalSym) != dbAttrNull) {
        return true; /* This is just a graphical thing */
    }
    return false;
}

/*--------------------------------------------------------------------------------------------------
  Determine if a name has a range, and what the range is.
--------------------------------------------------------------------------------------------------*/
bool dbNameHasRange(
    char *name,
    uint32 *left,
    uint32 *right)
{
    char *buf = utCopyString(name);
    char *leftPtr = strrchr(buf, '[');
    char *rightPtr, *p, *endLeftPtr, *endRightPtr;

    *left = 0;
    *right = 0;
    if(leftPtr == NULL) {
        return false;
    }
    leftPtr++;
    rightPtr = strchr(leftPtr, ':');
    if(rightPtr == NULL) {
        return false;
    }
    *rightPtr++ = '\0';
    p = strchr(rightPtr, ']');
    if(p == NULL) {
        return false;
    }
    *p++ = '\0';
    if(*p != '\0') {
        return false;
    }
    *left = strtol(leftPtr, &endLeftPtr, 10);
    *right = strtol(rightPtr, &endRightPtr, 10);
    if(*endLeftPtr != '\0' || *endRightPtr != '\0') {
        return false;
    }
    return true;
}
