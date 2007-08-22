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
  Write netlists back to the database.
--------------------------------------------------------------------------------------------------*/
#include <ctype.h>
#include <string.h>
#include "sch.h"

/*--------------------------------------------------------------------------------------------------
  Build a database attribute list from the attribute list.
--------------------------------------------------------------------------------------------------*/
static dbAttr buildDbAttrList(
    schAttr attr)
{
    dbAttr dbattr, dbchild;

    if(attr == schAttrNull) {
        return dbAttrNull;
    }
    dbattr = dbAttrCreate(schAttrGetName(attr), schAttrGetValue(attr));
    dbchild = buildDbAttrList(schAttrGetNextAttr(attr));
    dbAttrSetNextAttr(dbattr, dbchild);
    return dbattr;
}

/*--------------------------------------------------------------------------------------------------
  Find the name of the netlist the symbol represents.  Remove any -N.sym suffix.
--------------------------------------------------------------------------------------------------*/
static utSym findNetlistName(
    schSymbol symbol)
{
    char *name = utReplaceSuffix(schSymbolGetName(symbol), "");
    char *version = strrchr(name, '-');
    char *p;

    if(version != NULL) {
        p = version;
        do {
            ++p;
        } while(isdigit(*p));
        if(*p == '\0') {
            *version = '\0';
        }
    }
    return utSymCreate(name);
}

/*--------------------------------------------------------------------------------------------------
  Remove the [#:#] at the end of the bus mpin name.  Database mbusses don't have this.
--------------------------------------------------------------------------------------------------*/
static char *findMbusName(
    char *mpinName)
{
    char *buf = utCopyString(mpinName);
    char *p = strrchr(buf, '[');

    *p = '\0';
    return buf;
}

/*--------------------------------------------------------------------------------------------------
  Build an empty netlist with flags for the symbol's mpins, unless the netlist already exists in
  the library, in which case, bind it.
--------------------------------------------------------------------------------------------------*/
static void buildNetlistForSymbol(
    dbDesign design,
    dbDesign libDesign,
    schSymbol symbol)
{
    utSym device = schSymbolGetDevice(symbol);
    dbNetlistType type = schSymbolGetType(symbol);
    utSym name = findNetlistName(symbol);
    dbNetlist netlist = dbNetlistNull;
    dbMbus mbus;
    dbMport mport;
    schMpin mpin;
    utSym mbusName;

    if(libDesign != dbDesignNull) {
        netlist = dbDesignFindNetlist(libDesign, name);
    }
    if(netlist == dbNetlistNull) {
        netlist = dbNetlistCreate(design, name, type, device);
        dbNetlistSetAttr(netlist, buildDbAttrList(schSymbolGetAttr(symbol)));
        schForeachSymbolMpin(symbol, mpin) {
            if(schMpinBus(mpin)) {
                mbusName = utSymCreate(findMbusName(schMpinGetName(mpin)));
                mbus = dbMbusCreate(netlist, mbusName, schMpinGetType(mpin), schMpinGetLeft(mpin),
                    schMpinGetRight(mpin));
                schMpinSetMbus(mpin, mbus);
            } else {
                mport = dbMportCreate(netlist, schMpinGetSym(mpin), schMpinGetType(mpin));
                schMpinSetMport(mpin, mport);
            }
        } schEndSymbolMpin;
    } else {
        if(dbNetlistGetType(netlist) != type) {
            utWarning("Component %s has a different type than it's library definition: "
                "%s vs %s", utSymGetName(name), dbFindNetlistTypeName(type),
                dbFindNetlistTypeName(dbNetlistGetType(netlist)));
        }
        if(dbNetlistGetDevice(netlist) != device) {
            utWarning("Component %s has a different device specification than it's library"
                " definition", utSymGetName(name));
        }
        schForeachSymbolMpin(symbol, mpin) {
            if(schMpinBus(mpin)) {
                mbusName = utSymCreate(findMbusName(schMpinGetName(mpin)));
                mbus = dbNetlistFindMbus(netlist, mbusName);
                if(mbus == dbMbusNull) {
                    utWarning("Bus port %s not found on library component %s",
                        utSymGetName(mbusName), utSymGetName(name));
                }
                schMpinSetMbus(mpin, mbus);
            } else {
                mport = dbNetlistFindMport(netlist, schMpinGetSym(mpin));
                if(mport == dbMportNull) {
                    utWarning("Port %s not found on library component %s",
                        utSymGetName(schMpinGetSym(mpin)), utSymGetName(name));
                }
                schMpinSetMport(mpin, mport);
            }
        } schEndSymbolMpin;
        schSchemSetWritten(schSymbolGetSchem(symbol), true);
    }
    schSymbolSetNetlist(symbol, netlist);
}

/*--------------------------------------------------------------------------------------------------
  Write a bus port to the database.
--------------------------------------------------------------------------------------------------*/
static void writeBusPort(
    dbInst inst,
    schPin pin)
{
    dbMbus mbus = dbInstGetExternalMbus(inst);
    dbMport mport;
    dbPort port;
    bool firstTime = true;

    if(mbus == dbMbusNull) {
        mbus = schMpinGetMbus(schPinGetMpin(pin));
    }
    utAssert(mbus != dbMbusNull);
    dbForeachMbusMport(mbus, mport) {
        port = dbPortCreate(inst, mport);
        if(dbInstGetType(inst) == DB_FLAG) {
            dbMportSetFlagPort(mport, port);
            dbPortSetMport(port, mport);
        }
        if(firstTime) {
            firstTime = false;
            /* We set pins to point to the first port of a bus */
            schPinSetPort(pin, port);
        }
    } dbEndMbusMport;
}

/*--------------------------------------------------------------------------------------------------
  Write a port that connects a single signal to an array'ed instance port.  This creates several
  ports on the instance, one per signal in the net.  Each is owned by the same mport.
--------------------------------------------------------------------------------------------------*/
static void writeCompArrayPort(
    dbInst inst,
    schPin pin)
{
    schComp comp = schPinGetComp(pin);
    dbMport mport = schMpinGetMport(schPinGetMpin(pin));
    dbPort port;
    uint32 left = schCompGetLeft(comp);
    uint32 right = schCompGetRight(comp);
    uint32 xPin = left;
    bool firstTime = true;

    utDo {
        port = dbPortCreate(inst, mport);
        if(firstTime) {
            firstTime = false;
            /* We set pins to point to the first port of a bus */
            schPinSetPort(pin, port);
        }
    } utWhile(xPin != right) {
        if(xPin < right) {
            xPin++;
        } else {
            xPin--;
        }
    } utRepeat;
}

/*--------------------------------------------------------------------------------------------------
  Write a port that connects an exploded bus to an array'ed instance bus port.  This creates a
  number of signals equal to the bus width of the pin times the array width of the instance.
  The mports on the ports are in the mbus sequence, but repeated the array width times.
--------------------------------------------------------------------------------------------------*/
static void writeCompArrayExplodedBus(
    dbInst inst,
    schPin pin)
{
    schComp comp = schPinGetComp(pin);
    dbPort firstPort = dbPortNull;
    uint32 left = schCompGetLeft(comp);
    uint32 right = schCompGetRight(comp);
    uint32 xPin = left;
    bool firstTime = true;

    utDo {
        writeBusPort(inst, pin);
        if(firstTime) {
            firstTime = false;
            firstPort = schPinGetPort(pin);
        }
    } utWhile(xPin != right) {
        if(xPin < right) {
            xPin++;
        } else {
            xPin--;
        }
    } utRepeat;
    /* We set pins to point to the first port of the first bus */
    schPinSetPort(pin, firstPort);
}

/*--------------------------------------------------------------------------------------------------
  Set the owning mport or mbus of the flag instance.
--------------------------------------------------------------------------------------------------*/
static void setMportFlagPort(
    dbPort port,
    schComp comp)
{
    schMpin mpin = schCompGetMpin(comp);
    dbMport mport = schMpinGetMport(mpin);

    if(mport != dbMportNull) {
        dbMportSetFlagPort(mport, port);
        dbPortSetMport(port, mport);
    }
}

/*--------------------------------------------------------------------------------------------------
  Set the owning mport or mbus of the flag instance.
--------------------------------------------------------------------------------------------------*/
static void setInstMbus(
    dbInst inst,
    schComp comp)
{
    schMpin mpin = schCompGetMpin(comp);
    dbMbus mbus = schMpinGetMbus(mpin);

    if(mbus != dbMbusNull) {
        dbInstSetExternalMbus(inst, mbus);
        dbMbusSetFlagInst(mbus, inst);
    }
}

/*--------------------------------------------------------------------------------------------------
  Write a port to the database.
--------------------------------------------------------------------------------------------------*/
static void writePort(
    dbInst inst,
    schPin pin)
{
    schMpin mpin = schPinGetMpin(pin);
    dbMport mport = dbNetlistFindMport(dbInstGetInternalNetlist(inst), schMpinGetSym(mpin));
    dbPort port;

    port = dbPortCreate(inst, mport);
    schPinSetPort(pin, port);
    if(dbInstGetType(inst) == DB_FLAG) {
        setMportFlagPort(port, schPinGetComp(pin));
    }
}

/*--------------------------------------------------------------------------------------------------
  Determine if this pin is a single signal wide, but on a non-flag array instance and attached
  to a bus net.
--------------------------------------------------------------------------------------------------*/
static bool pinIsNonBusOnArrayInstAttachedToBus(
    schPin pin)
{
    schComp comp = schPinGetComp(pin);
    schNet net = schPinGetNet(pin);
    schMpin mpin = schPinGetMpin(pin);

    if(!schCompArray(comp)) {
        return false;
    }
    if(net == schNetNull || !schNetBus(net)) {
        return false;
    }
    if(schMpinBus(mpin)) {
        return false;
    }
    return true;
}

/*--------------------------------------------------------------------------------------------------
  Count the number of signals on the net.
--------------------------------------------------------------------------------------------------*/
static uint32 countNetSignals(
    schNet net)
{
    schSignal signal;
    uint32 numSignals = 0;

    schForeachNetSignal(net, signal) {
        numSignals++;
    } schEndNetSignal;
    return numSignals;
}

/*--------------------------------------------------------------------------------------------------
  Determine if this pin is a bus on a non-flag array instance and attached
  to an exploded bus net.  An exploded bus net will have width = instance width * pin width.
--------------------------------------------------------------------------------------------------*/
static bool pinIsBusOnArrayInstAttachedToExplodedBus(
    schPin pin)
{
    schComp comp = schPinGetComp(pin);
    schNet net = schPinGetNet(pin);
    schMpin mpin = schPinGetMpin(pin);
    uint32 numSignals, compWidth, pinWidth;

    if(!schCompArray(comp)) {
        return false;
    }
    if(net == schNetNull || !schNetBus(net)) {
        return false;
    }
    if(!schMpinBus(mpin)) {
        return false;
    }
    numSignals = countNetSignals(net);
    compWidth = utAbs((int32)schCompGetRight(comp) - (int32)schCompGetLeft(comp)) + 1;
    pinWidth = utAbs((int32)schMpinGetRight(mpin) - (int32)schMpinGetLeft(mpin)) + 1;
    if(numSignals != compWidth*pinWidth) {
        return false;
    }
    return true;
}

/*--------------------------------------------------------------------------------------------------
  Write an instance to the netlist for the component.
--------------------------------------------------------------------------------------------------*/
static void writeInst(
    dbNetlist netlist,
    schComp comp,
    dbNetlist internalNetlist)
{
    utSym name = schCompGetSym(comp);
    dbInst inst = dbInstCreate(netlist, name, internalNetlist);
    schPin pin;

    dbInstSetAttr(inst, buildDbAttrList(schCompGetAttr(comp)));
    if(schCompArray(comp)) {
        dbInstSetArray(inst, true);
    }
    if(dbNetlistGetType(internalNetlist) == DB_FLAG) {
        setInstMbus(inst, comp);
    }
    schForeachCompPin(comp, pin) {
        if(pinIsNonBusOnArrayInstAttachedToBus(pin)) {
            writeCompArrayPort(inst, pin);
        } else if(pinIsBusOnArrayInstAttachedToExplodedBus(pin)) {
            writeCompArrayExplodedBus(inst, pin);
        } else if(schPinIsBus(pin)) {
            writeBusPort(inst, pin);
        } else {
            writePort(inst, pin);
        }
    } schEndCompPin;
}

/*--------------------------------------------------------------------------------------------------
  Write instances to the netlist for each component.
--------------------------------------------------------------------------------------------------*/
static void writeInsts(
    dbNetlist netlist,
    schSchem schem)
{
    schSymbol symbol;
    schComp comp;

    schForeachSchemComp(schem, comp) {
        symbol = schCompGetSymbol(comp);
        writeInst(netlist, comp, schSymbolGetNetlist(symbol));
    } schEndSchemComp;
}

/*--------------------------------------------------------------------------------------------------
  Write the bus-net to the netlist.
--------------------------------------------------------------------------------------------------*/
static void attachBusNetToInstPorts(
    dbNetlist netlist,
    schNet net,
    schPin pin)
{
    dbPort port = schPinGetPort(pin);
    dbMport mport = dbPortGetMport(port);
    dbMbus mbus = dbMportGetMbus(mport);
    schSignal signal;
    dbNet dbnet;
    utSym name;

    schForeachNetSignal(net, signal) {
        if(port == dbPortNull ||
                (mbus == dbMbusNull? dbPortGetMport(port) != mport :
            dbMportGetMbus(dbPortGetMport(port)) != mbus)) {
            utWarning("Bus width mismatch found on pin %s on component %s in schematic %s",
                schMpinGetName(schPinGetMpin(pin)), schCompGetUserName(schPinGetComp(pin)),
                schSchemGetName(schNetGetSchem(net)));
            return;
        }
        name = schSignalGetSym(signal);
        dbnet = dbNetlistFindNet(netlist, name);
        if(dbnet == dbNetNull) {
            /* Some nets in busses are not bus signals */
            dbnet = dbNetCreate(netlist, name);
        }
        dbNetInsertPort(dbnet, port);
        port = dbPortGetNextInstPort(port);
    } schEndNetSignal;
    if(port != dbPortNull &&
            (mbus == dbMbusNull? dbPortGetMport(port) == mport :
            dbMportGetMbus(dbPortGetMport(port)) == mbus)) {
        utWarning("Bus width mismatch found on pin %s on component %s in schematic %s",
            schMpinGetName(schPinGetMpin(pin)), schCompGetUserName(schPinGetComp(pin)),
            schSchemGetName(schNetGetSchem(net)));
    }
}

/*--------------------------------------------------------------------------------------------------
  Write the bus-net to the netlist.
--------------------------------------------------------------------------------------------------*/
static void writeBusNet(
    dbNetlist netlist,
    schNet net)
{
    schPin pin;

    schForeachNetPin(net, pin) {
        if(!schPinIsBus(pin)) {
            utWarning("Single signal pin %s on component %s in schematic %s is connected to a bus",
                schMpinGetName(schPinGetMpin(pin)), schCompGetUserName(schPinGetComp(pin)),
                schSchemGetName(schNetGetSchem(net)));
        } else {
            attachBusNetToInstPorts(netlist, net, pin);
        }
    } schEndNetPin;
}

/*--------------------------------------------------------------------------------------------------
  Write the net to the netlist.
--------------------------------------------------------------------------------------------------*/
static void writeNet(
    dbNetlist netlist,
    schNet net)
{
    dbNet dbnet = dbNetlistFindNet(netlist, schNetGetSym(net));
    schPin pin;
    dbPort port;

    if(dbnet == dbNetNull) {
        /* Some nets are parts of busses, and were already built */
        dbnet = dbNetCreate(netlist, schNetGetSym(net));
    }
    dbNetSetAttr(dbnet, buildDbAttrList(schNetGetAttr(net)));
    schForeachNetPin(net, pin) {
        port = schPinGetPort(pin);
        dbNetInsertPort(dbnet, port);
    } schEndNetPin;
}

/*--------------------------------------------------------------------------------------------------
  Write nets in the schematic to the netlist.
--------------------------------------------------------------------------------------------------*/
static void writeNets(
    dbNetlist netlist,
    schSchem schem)
{
    schNet net;

    schForeachSchemNet(schem, net) {
        if(schNetBus(net)) {
            writeBusNet(netlist, net);
        } else {
            writeNet(netlist, net);
        }
    } schEndSchemNet;
}

/*--------------------------------------------------------------------------------------------------
  Build busses in the netlist database.
--------------------------------------------------------------------------------------------------*/
static void writeBusses(
    dbNetlist netlist,
    schSchem schem)
{
    schBus bus;

    schForeachSchemBus(schem, bus) {
        dbBusCreate(netlist, schBusGetSym(bus), schBusGetLeft(bus), schBusGetRight(bus));
    } schEndSchemBus;
}

/*--------------------------------------------------------------------------------------------------
  Write the schematic to the database as a netlist.  Write any child schems first.
--------------------------------------------------------------------------------------------------*/
static dbNetlist writeSchems(
    dbDesign design,
    schSchem schem)
{
    schSymbol symbol = schSchemGetSymbol(schem);
    schSchem childSchem;
    dbNetlist netlist = schSymbolGetNetlist(symbol);
    schComp comp;
    schAttr attr;
    dbAttr dbattr;

    schSchemSetWritten(schem, true);
    for(attr = schSchemGetAttr(schem); attr != schAttrNull; attr = schAttrGetNextAttr(attr)) {
        dbattr = dbAttrCreate(schAttrGetName(attr), schAttrGetValue(attr));
        dbAttrSetNextAttr(dbattr, dbNetlistGetAttr(netlist));
        dbNetlistSetAttr(netlist, dbattr);
    }
    schForeachSchemComp(schem, comp) {
        symbol = schCompGetSymbol(comp);
        childSchem = schSymbolGetSchem(symbol);
        if(childSchem != schSchemNull && !schSchemWritten(childSchem)) {
            writeSchems(design, childSchem);
        }
    } schEndSchemComp;
    writeInsts(netlist, schem);
    writeBusses(netlist, schem);
    writeNets(netlist, schem);
    return netlist;
}

/*--------------------------------------------------------------------------------------------------
  Build netlists in the database design.  Every schem needs to have a symbol.
--------------------------------------------------------------------------------------------------*/
bool schBuildNetlists(
    dbDesign design,
    dbDesign libDesign,
    schSchem rootSchem)
{
    schSymbol symbol;

    schForeachRootSymbol(schTheRoot, symbol) {
        buildNetlistForSymbol(design, libDesign, symbol);
    } schEndRootSymbol;
    writeSchems(design, rootSchem);
    dbDesignBuildNetsForFloatingPorts(design);
    return true;
}

