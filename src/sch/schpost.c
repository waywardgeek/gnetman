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
  Routines manipulating schematics.
--------------------------------------------------------------------------------------------------*/
#include <ctype.h>
#include <string.h>
#include "sch.h"

/*--------------------------------------------------------------------------------------------------
  Allocate memory used in the schematic manipulation module.
--------------------------------------------------------------------------------------------------*/
void schStartSchem(void)
{
    /* Nothing for now */
}

/*--------------------------------------------------------------------------------------------------
  Free memory used in the schematic manipulation module.
--------------------------------------------------------------------------------------------------*/
void schStopSchem(void)
{
    /* Nothing for now */
}


/*--------------------------------------------------------------------------------------------------
  Load all of the symbols used by a schematic.
--------------------------------------------------------------------------------------------------*/
static bool schemLoadSymbols(
    schSchem schem,
    bool loadSubSchems)
{
    schComp comp;
    schSymbol symbol;
    utSym name;
    char *fileName;

    schForeachSchemComp(schem, comp) {
        utAssert(schCompGetSymbol(comp) == schSymbolNull);
        name = schCompGetSymbolName(comp);
        utAssert(name != utSymNull);
        symbol = schRootFindSymbol(schTheRoot, name);
        if(symbol == schSymbolNull) {
            fileName = utSprintf("%s%c%s", utSymGetName(schSchemGetPath(schem)), UTDIRSEP,
                utSymGetName(name));
            if(!utAccess(fileName, "r")) {
                fileName = utFindInPath(utSymGetName(name), dbGschemComponentPath);
                if(fileName == NULL) {
                    utWarning("Could not find symbol %s from schematic %s, component %s\n"
                        "Search path = %s", utSymGetName(name), schSchemGetName(schem),
                        schCompGetUserName(comp), dbGschemComponentPath);
                    return false;
                }
            }
            symbol = schReadSymbol(fileName, loadSubSchems);
            if(symbol == schSymbolNull) {
                return false;
            }
        }
        schSymbolInsertComp(symbol, comp);
    } schEndSchemComp;
    return true;
}

/*--------------------------------------------------------------------------------------------------
  Set the internal flag comps on the mpins of the symbol.
--------------------------------------------------------------------------------------------------*/
static void setMpinFlagComps(
    schSymbol symbol)
{
    schSchem schem = schSymbolGetSchem(symbol);
    schMpin mpin;
    schComp comp;
    utSym name;
    
    schForeachSchemComp(schem, comp) {
        if(schCompGetSym(comp) == utSymNull) {
            name = schFindAttrValue(schCompGetAttr(comp), schValueSym);
            if(name == utSymNull) {
                utWarning("Unnamed flag found in schematic %s", schSchemGetName(schem));
            } else {
                mpin = schSymbolFindMpin(symbol, name);
                if(mpin == schMpinNull) {
                    utWarning("No pin %s found on symbol %s", utSymGetName(name),
                        schSymbolGetName(symbol));
                } else {
                    schMpinSetFlagComp(mpin, comp);
                    schCompSetMpin(comp, mpin);
                    schCompSetArray(comp, schMpinBus(mpin));
                    schCompSetLeft(comp, schMpinGetLeft(mpin));
                    schCompSetRight(comp, schMpinGetRight(mpin));
                }
            }
        }
    } schEndSchemComp;
    schForeachSymbolMpin(symbol, mpin) {
        if(schMpinGetFlagComp(mpin) == schCompNull) {
            utWarning("No flag %s found in schematic %s", schMpinGetName(mpin),
                schSchemGetName(schem));
        }
    } schEndSymbolMpin;
}

/*--------------------------------------------------------------------------------------------------
  Load the underlying schematic for the symbol.
--------------------------------------------------------------------------------------------------*/
static bool symbolLoadSchem(
    schSymbol symbol,
    bool loadSubSchems)
{
    schSchem schem;
    utSym name;
    char *fileName;

    utAssert(schSymbolGetSchem(symbol) == schSchemNull);
    name = schFindAttrValue(schSymbolGetAttr(symbol), schSourceSym);
    if(name == utSymNull) {
        return true;
    }
    schem = schRootFindSchem(schTheRoot, name);
    if(schem == schSchemNull) {
        fileName = utSprintf("%s%c%s", utSymGetName(schSymbolGetPath(symbol)), UTDIRSEP,
            utSymGetName(name));
        if(!utAccess(fileName, "r")) {
            fileName = utFindInPath(utSymGetName(name), dbGschemSourcePath);
            if(fileName == NULL) {
                utWarning("Could not find schematic %s for symbol %s\nSearch path = %s",
                    utSymGetName(name), schSymbolGetName(symbol), dbGschemSourcePath);
                return false;
            }
        }
        schem = schReadSchem(fileName, loadSubSchems);
        if(schem == schSchemNull) {
            return false;
        }
        schSymbolSetSchem(symbol, schem);
        schSchemSetSymbol(schem, symbol);
        setMpinFlagComps(symbol);
    }
    return true;
}

/*--------------------------------------------------------------------------------------------------
  Build pins on the component.  The component should already have an owning symbol.
--------------------------------------------------------------------------------------------------*/
static void buildCompPins(
    schComp comp)
{
    schSymbol symbol = schCompGetSymbol(comp);
    schMpin mpin;

    utAssert(symbol != schSymbolNull && schCompGetFirstPin(comp) == schPinNull);
    schForeachSymbolMpin(symbol, mpin) {
        schPinCreate(comp, mpin);
    } schEndSymbolMpin;
}

/*--------------------------------------------------------------------------------------------------
  Build pins for all components in the schematic.
--------------------------------------------------------------------------------------------------*/
static void buildPins(
    schSchem schem)
{
    schComp comp;

    schForeachSchemComp(schem, comp) {
        buildCompPins(comp);
    } schEndSchemComp;
}

/*--------------------------------------------------------------------------------------------------
  Build nets from wires in the schematic.
--------------------------------------------------------------------------------------------------*/
schConn schFindConnFromWires(
    schWire wire1,
    schWire wire2)
{
    schConn conn;

    schForeachWireConn(wire1, conn) {
        if(schConnFindOtherWire(conn, wire1) == wire2) {
            return conn;
        }
    } schEndWireConn;
    return schConnNull;
}

/*--------------------------------------------------------------------------------------------------
  Find the other wire at the point on the wire.  Only find wires with no connections to this wire.

  Note: speed this up with sorted vertical and horizontal arrays, and end-point hash tables.
--------------------------------------------------------------------------------------------------*/
static schWire findOtherWireAtPoint(
    schSchem schem,
    schWire wire,
    int32 x,
    int32 y)
{
    schWire otherWire;
    utBox box;

    schForeachSchemWire(schem, otherWire) {
        if(wire == schWireNull || otherWire != wire) {
            box = schWireFindBox(otherWire);
            if(utBoxIsHorLine(box) || utBoxIsVertLine(box)) {
                if(utBoxContainsPoint(box, x, y) && (wire == schWireNull ||
                        schFindConnFromWires(wire, otherWire) == schConnNull)) {
                    return otherWire;
                }
            } else {
                if(schWireGetX1(otherWire) == x && schWireGetY1(otherWire) == y &&
                        (wire == schWireNull ||
                        schFindConnFromWires(wire, otherWire) == schConnNull)) {
                    return otherWire;
                }
                if(schWireGetX2(otherWire) == x && schWireGetY2(otherWire) == y &&
                        (wire == schWireNull ||
                        schFindConnFromWires(wire, otherWire) == schConnNull)) {
                    return otherWire;
                }
            }
        }
    } schEndSchemWire;
    return schWireNull;
}

/*--------------------------------------------------------------------------------------------------
  Build nets from wires in the schematic.
--------------------------------------------------------------------------------------------------*/
static void connectWires(
    schSchem schem)
{
    schWire wire, otherWire;
    int32 x, y;

    schForeachSchemWire(schem, wire) {
        x = schWireGetX1(wire);
        y = schWireGetY1(wire);
        otherWire = findOtherWireAtPoint(schem, wire, x, y);
        if(otherWire != schWireNull) {
            schConnCreate(wire, otherWire, x, y);
        }
        x = schWireGetX2(wire);
        y = schWireGetY2(wire);
        otherWire = findOtherWireAtPoint(schem, wire, x, y);
        if(otherWire != schWireNull) {
            schConnCreate(wire, otherWire, x, y);
        }
    } schEndSchemWire;
}

/*--------------------------------------------------------------------------------------------------
  Add wires to the net from this wire.  Traverse through conns.
--------------------------------------------------------------------------------------------------*/
static void addWiresToNet(
    schNet net,
    schWire wire)
{
    schWire otherWire;
    schConn conn;
    schNet otherNet;
    bool isBus = schWireBus(wire);

    schNetInsertWire(net, wire);
    schForeachWireConn(wire, conn) {
        otherWire = schConnFindOtherWire(conn, wire);
        if(schWireBus(otherWire) == isBus) {
            otherNet = schWireGetNet(otherWire);
            if(otherNet == schNetNull) {
                addWiresToNet(net, otherWire);
            } else if(otherNet != net) {
                utWarning("Net %s is shorted to net %s at (%d, %d)", schNetGetName(net),
                    schNetGetName(otherNet), schConnGetX(conn), schConnGetY(conn));
            }
        }
    } schEndWireConn;
}

/*--------------------------------------------------------------------------------------------------
  Find if a net connection can be inferred from a net attribute on the component or it's symbol.
--------------------------------------------------------------------------------------------------*/
static schNet findNetFromNetAttr(
    schPin pin)
{
    schComp comp = schPinGetComp(pin);
    schSchem schem = schCompGetSchem(comp);
    schSymbol symbol = schCompGetSymbol(comp);
    utSym netAssignments = schFindAttrValue(schCompGetAttr(comp), schNetSym);
    schNet net;
    char *pinName, *buffer, *nextFieldPtr;
    char *mpinName = schMpinGetName(schPinGetMpin(pin));
    utSym netSym;

    if(netAssignments == utSymNull) {
        netAssignments = schFindAttrValue(schSymbolGetAttr(symbol), schNetSym);
    }
    if(netAssignments == utSymNull) {
        return schNetNull;
    }
    buffer = utCopyString(utSymGetName(netAssignments));
    nextFieldPtr = strchr(buffer, ':');
    if(nextFieldPtr == NULL) {
        utWarning("Bad net attribute format: %s", buffer);
        return schNetNull;
    }
    *nextFieldPtr++ = '\0';
    netSym = utSymCreate(buffer);
    while(nextFieldPtr != NULL) {
        pinName = nextFieldPtr;
        nextFieldPtr = strchr(pinName, ',');
        if(nextFieldPtr != NULL) {
            *nextFieldPtr++ = '\0';
        }
        if(!strcmp(pinName, mpinName)) {
            net = schSchemFindNet(schem, netSym);
            if(net != schNetNull) {
                return net;
            }
            return schNetCreate(schem, netSym, false);
        }
    }
    return schNetNull;
}

/*--------------------------------------------------------------------------------------------------
  Add pins to nets they touch.  Be sure to build all nets for wires before calling this.

  Note: speed this up with hash tables and sorted arrays
--------------------------------------------------------------------------------------------------*/
static void addPinsToNets(
    schSchem schem)
{
    schComp comp;
    schPin pin;
    schNet net;
    schWire wire;

    schForeachSchemComp(schem, comp) {
        schForeachCompPin(comp, pin) {
             wire = findOtherWireAtPoint(schem, schWireNull, schPinGetX(pin), schPinGetY(pin));
             if(wire != schWireNull) {
                 net = schWireGetNet(wire);
                 utAssert(net != schNetNull);
                 schNetAppendPin(net, pin);
             } else {
                 net = findNetFromNetAttr(pin);
                 if(net != schNetNull) {
                     schNetAppendPin(net, pin);
                 }
             }
        } schEndCompPin;
    } schEndSchemComp;
}

/*--------------------------------------------------------------------------------------------------
  We copy flag names to unnamed nets long after we've built all the nets.  Generated names have
  allready been given to unnamed nets.  If a flag name collides with a generated name, we need to
  rename the generated net so it wont short to the net with a flag.
--------------------------------------------------------------------------------------------------*/
static void renameCollidingGeneratedNet(
    schSchem schem,
    utSym netName)
{
    schNet net = schSchemFindNet(schem, netName);

    if(net != schNetNull && schNetNameGenerated(net)) {
        netName = schSchemCreateUniqueNetName(schem, "N");
        schNetRename(net, netName);
    }
}

/*--------------------------------------------------------------------------------------------------
  Set the name of the net to that of it's first flag, if it exists.
--------------------------------------------------------------------------------------------------*/
static void setNetNameToFlagName(
    schNet net)
{
    schSchem schem = schNetGetSchem(net);
    schSymbol symbol;
    schComp comp;
    schPin pin;
    schWire wire;
    utSym netName;

    schForeachNetPin(net, pin) {
        comp = schPinGetComp(pin);
        symbol = schCompGetSymbol(comp);
        if(schSymbolGetType(symbol) == DB_FLAG) {
            netName = schFindAttrValue(schCompGetAttr(comp), schValueSym);
            if(netName != utSymNull) {
                renameCollidingGeneratedNet(schem, netName);
                schNetRename(net, netName);
                schNetSetNameGenerated(net, false);
                return;
            }
        }
    } schEndNetPin;
    if(schNetBus(net) && schNetNameGenerated(net)) {
        wire = schNetGetFirstWire(net);
        utWarning("Unnamed bus found in schematic %s at (%d, %d)", schSchemGetName(schem),
            schWireGetX1(wire), schWireGetY2(wire));
    }
}

/*--------------------------------------------------------------------------------------------------
  Change the names of unnamed nets to use the names of flags they attach to.
--------------------------------------------------------------------------------------------------*/
static void setUnnamedNetsToFlagNames(
    schSchem schem)
{
    schNet net;

    schSafeForeachSchemNet(schem, net) {
        if(schNetNameGenerated(net)) {
            setNetNameToFlagName(net);
        }
    } schEndSafeForeachSchemNet;
}

/*--------------------------------------------------------------------------------------------------
  Build nets from wires in the schematic.
--------------------------------------------------------------------------------------------------*/
static void buildNets(
    schSchem schem)
{
    schNet net;
    schWire wire;
    utSym netName;
    bool isBus;

    connectWires(schem);
    /* First, build named nets */
    schForeachSchemWire(schem, wire) {
        if(schWireGetNet(wire) == schNetNull) {
            netName = schFindAttrValue(schWireGetAttr(wire), schNetnameSym);
            if(netName != utSymNull) {
                net = schNetCreate(schem, netName, schWireBus(wire));
                addWiresToNet(net, wire);
            }
        }
    } schEndSchemWire;
    /* Now build unnamed nets */
    schForeachSchemWire(schem, wire) {
        if(schWireGetNet(wire) == schNetNull) {
            netName = schSchemCreateUniqueNetName(schem, "N");
            isBus = schWireBus(wire);
            net = schNetCreate(schem, netName, isBus);
            schNetSetNameGenerated(net, true);
            addWiresToNet(net, wire);
        }
    } schEndSchemWire;
    addPinsToNets(schem);
    setUnnamedNetsToFlagNames(schem);
}

/*--------------------------------------------------------------------------------------------------
  Create the signals for a bus range.
--------------------------------------------------------------------------------------------------*/
static void buildBusSignals(
    schNet net,
    char *name,
    uint32 left,
    uint32 right)
{
    schSchem schem = schNetGetSchem(net);
    utSym busName = utSymCreate(name);
    schBus bus = schSchemFindBus(schem, busName);
    uint32 xSignal = left;
    uint32 busLeft, busRight;

    if(bus == schBusNull) {
        schBusCreate(schem, busName, left, right);
    } else {
        /* Keep same bit order in bus */
        busLeft = schBusGetLeft(bus);
        busRight = schBusGetRight(bus);
        if(busLeft > busRight || (busLeft == busRight && left >= right)) {
            schBusSetLeft(bus, utMax(busLeft, left));
            schBusSetRight(bus, utMin(busRight, right));
        } else {
            schBusSetLeft(bus, utMin(busLeft, left));
            schBusSetRight(bus, utMax(busRight, right));
        }
    }
    utDo {
        schSignalCreate(net, bus, utSymCreateFormatted("%s[%u]", name, xSignal));
    } utWhile(xSignal != right) {
        if(xSignal < right) {
            xSignal++;
        } else {
            xSignal--;
        }
    } utRepeat;
}

/*--------------------------------------------------------------------------------------------------
  Build signals for all the bits of a bus net.  This is where we parse the commas and brackets.
--------------------------------------------------------------------------------------------------*/
static bool buildNetSignals(
    schNet net)
{
    char *buf = utAllocString(schNetGetName(net));
    char *p, *q;
    uint32 left, right;
    uint32 numSignals = 0;
    
    utDo {
        p = strchr(buf, ',');
        if(p != NULL) {
            *p++ = '\0';
        }
        if(dbNameHasRange(buf, &left, &right)) {
            q = strchr(buf, '[');
            *q = '\0';
            buildBusSignals(net, buf, left, right);
            numSignals += utAbs((int32)left - (int32)right) + 1;
        } else {
            schSignalCreate(net, schBusNull, utSymCreate(buf));
            numSignals++;
        }
    } utWhile(p != NULL) {
        buf = p;
    } utRepeat;
    if(numSignals > 1 && !schNetBus(net)) {
        utWarning("Non-bus net %s has multiple signals", schNetGetName(net));
        return false;
    }
    if(numSignals == 1 && schNetBus(net)) {
        utWarning("Bus net %s has only one signal", schNetGetName(net));
        return false;
    }
    utFree(buf);
    return true;
}

/*--------------------------------------------------------------------------------------------------
  Build signals for all the nets.
--------------------------------------------------------------------------------------------------*/
static bool buildSignals(
    schSchem schem)
{
    schNet net;

    schForeachSchemNet(schem, net) {
        if(!buildNetSignals(net)) {
            return false;
        }
    } schEndSchemNet;
    return true;
}

/*--------------------------------------------------------------------------------------------------
  Generate a unique name for each unnamed component.
--------------------------------------------------------------------------------------------------*/
static void nameUnnamedComps(
    schSchem schem)
{
    schSymbol symbol;
    schComp comp;
    utSym name;
    
    schSafeForeachSchemComp(schem, comp) {
        if(schCompGetSym(comp) == utSymNull) {
            symbol = schCompGetSymbol(comp);
            if(schSymbolGetDevice(symbol) != schFlagSym) {
                name = schSchemCreateUniqueCompName(schem, "U");
                schSchemRemoveComp(schem, comp);
                schCompSetSym(comp, name);
                schSchemInsertComp(schem, comp);
            }
        }
    } schEndSafeForeachSchemComp;
}

/*--------------------------------------------------------------------------------------------------
  Post-process a schematic.  Load symbols, and build nets and pins.
--------------------------------------------------------------------------------------------------*/
bool schSchemPostProcess(
    schSchem schem,
    bool loadSubSchems)
{
    if(!schemLoadSymbols(schem, loadSubSchems)) {
        return false;
    }
    nameUnnamedComps(schem);
    buildPins(schem);
    buildNets(schem);
    if(!buildSignals(schem)) {
        return false;
    }
    return true;
}

/*--------------------------------------------------------------------------------------------------
  Post-process a schematic.  Load symbols, and build nets and pins.
--------------------------------------------------------------------------------------------------*/
bool schSymbolPostProcess(
    schSymbol symbol,
    bool loadSubSchems)
{
    if(!symbolLoadSchem(symbol, loadSubSchems)) {
        return false;
    }
    schSymbolSetType(symbol, schSymbolFindType(symbol));
    return true;
}
