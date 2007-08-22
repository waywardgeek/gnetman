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
  Constructors for database classes.
  
  For nets and instances, names have a special case.  If they start with '.', then they are
  generated internal names, which should not show up in the output netlist.  If a real net or
  instance name starts with a '.', be sure to insert a \ in front.  Netlisters need to know to
  strip the leading \.
--------------------------------------------------------------------------------------------------*/
#include <string.h>
#include "db.h"

/*--------------------------------------------------------------------------------------------------
  Build globals for zero and one.
--------------------------------------------------------------------------------------------------*/
static void buildDefaultZeroAndOneGlobals(
    dbDesign design)
{
    dbGlobal one = dbGlobalCreate(design, dbDefaultOneSym);
    dbGlobal zero = dbGlobalCreate(design, dbDefaultZeroSym);

    dbDesignSetOneGlobal(design, one);
    dbDesignSetZeroGlobal(design, zero);
}

/*--------------------------------------------------------------------------------------------------
  Construct a new design object.
--------------------------------------------------------------------------------------------------*/
dbDesign dbDesignCreate(
    utSym name,
    dbDesign libraryDesign)
{
    dbDesign design = dbDesignAlloc();

    dbDesignSetSym(design, name);
    dbRootInsertDesign(dbTheRoot, design);
    if(libraryDesign != dbDesignNull) {
        dbDesignInsertLinkedDesign(libraryDesign, design);
    }
    buildDefaultZeroAndOneGlobals(design);
    return design;
}

/*--------------------------------------------------------------------------------------------------
  Construct a new netlist object.
--------------------------------------------------------------------------------------------------*/
dbNetlist dbNetlistCreate(
    dbDesign design,
    utSym name,
    dbNetlistType type,
    utSym device)
{
    dbNetlist netlist = dbNetlistAlloc();

    dbNetlistSetSym(netlist, name);
    dbNetlistSetType(netlist, type);
    dbNetlistSetDevice(netlist, device);
    dbDesignAppendNetlist(design, netlist);
    return netlist;
}

/*--------------------------------------------------------------------------------------------------
  Construct a new mport object.
--------------------------------------------------------------------------------------------------*/
dbMport dbMportCreate(
    dbNetlist netlist,
    utSym name,
    dbMportType type)
{
    dbMport mport;

    if(name != utSymNull) {
        mport = dbNetlistFindMport(netlist, name);
        if(mport != dbMportNull) {
            utExit("dbMportCreate: mport %s already defined", utSymGetName(name));
        }
    }
    mport = dbMportAlloc();
    dbMportSetSym(mport, name);
    dbMportSetType(mport, type);
    dbNetlistAppendMport(netlist, mport);
    return mport;
}

/*--------------------------------------------------------------------------------------------------
  Find a flag netlist in the design with one mport.  If non exists, create it.
--------------------------------------------------------------------------------------------------*/
static dbNetlist findOrCreateFlagNetlist(
    dbMport mport)
{
    dbDesign design = dbNetlistGetDesign(dbMportGetNetlist(mport));
    dbNetlist netlist;
    dbMport flagMport;
    dbMportType type = dbMportGetType(mport);
    utSym name;

    dbForeachDesignNetlist(design, netlist) {
        if(dbNetlistGetType(netlist) == DB_FLAG) {
            flagMport = dbNetlistGetFirstMport(netlist);
            if(flagMport != dbMportNull && dbMportGetNextNetlistMport(flagMport) == dbMportNull) {
                if(dbMportGetType(flagMport) == type) {
                    return netlist;
                }
            }
        }
    } dbEndDesignNetlist;
    name = dbDesignCreateUniqueNetlistName(design,
        utSprintf("%s_flag", utSymGetName(dbFindPinTypeSym(type))));
    netlist = dbNetlistCreate(design, name, DB_FLAG, utSymNull);
    dbMportCreate(netlist, utSymNull, type);
    return netlist;
}

/*--------------------------------------------------------------------------------------------------
  Create a flag instance, with one port.  Also build an internal netlist for it.
--------------------------------------------------------------------------------------------------*/
dbInst dbFlagInstCreate(
    dbMport mport)
{
    dbNetlist flagNetlist = findOrCreateFlagNetlist(mport);
    dbInst inst = dbInstCreate(dbMportGetNetlist(mport), utSymNull, flagNetlist);
    dbPort port = dbPortCreate(inst, mport);

    utAssert(dbMportGetFlagPort(mport) == dbPortNull);
    dbMportSetFlagPort(mport, port);
    return inst;
}

/*--------------------------------------------------------------------------------------------------
  Find a flag netlist in the design with the mbus.  If non exists, create it.
--------------------------------------------------------------------------------------------------*/
static dbNetlist findOrCreateBusFlagNetlist(
    dbMbus mbus)
{
    dbDesign design = dbNetlistGetDesign(dbMbusGetNetlist(mbus));
    dbNetlist netlist;
    dbMbus flagMbus;
    dbMportType type = dbMbusGetType(mbus);
    utSym name;
    uint32 left = dbMbusGetLeft(mbus);
    uint32 right = dbMbusGetRight(mbus);

    dbForeachDesignNetlist(design, netlist) {
        if(dbNetlistGetType(netlist) == DB_FLAG) {
            flagMbus = dbNetlistGetFirstMbus(netlist);
            if(flagMbus != dbMbusNull && dbMbusGetNextNetlistMbus(flagMbus) == dbMbusNull) {
                if(dbMbusGetType(flagMbus) == type &&
                        dbMbusGetLeft(flagMbus) == left && dbMbusGetRight(flagMbus) == right) {
                    return netlist;
                }
            }
        }
    } dbEndDesignNetlist;
    name = dbDesignCreateUniqueNetlistName(design,
        utSprintf("%s_busflag", utSymGetName(dbFindPinTypeSym(type))));
    netlist = dbNetlistCreate(design, name, DB_FLAG, utSymNull);
    dbMbusCreate(netlist, utSymNull, type, left, right);
    return netlist;
}

/*--------------------------------------------------------------------------------------------------
  Create a flag instance, with the mbus.  Also build an internal netlist for it.
--------------------------------------------------------------------------------------------------*/
dbInst dbBusFlagInstCreate(
    dbMbus mbus)
{
    dbNetlist flagNetlist = findOrCreateBusFlagNetlist(mbus);
    dbInst inst = dbInstCreate(dbMbusGetNetlist(mbus), utSymNull, flagNetlist);
    dbMport mport;
    dbPort port;

    dbForeachMbusMport(mbus, mport) {
        port = dbPortCreate(inst, mport);
        dbMportSetFlagPort(mport, port);
    } dbEndMbusMport;
    return inst;
}

/*--------------------------------------------------------------------------------------------------
  Explode nets in the bus.
--------------------------------------------------------------------------------------------------*/
static void explodeMbus(
    dbMbus mbus,
    dbMportType type)
{
    dbNetlist netlist = dbMbusGetNetlist(mbus);
    dbMport mport;
    utSym name = dbMbusGetSym(mbus);
    uint32 left = dbMbusGetLeft(mbus);
    uint32 right = dbMbusGetRight(mbus);
    uint32 xMport = left;
    uint32 index = 0;

    dbMbusAllocMports(mbus, utAbs((int32)(right - left)) + 1);
    utDo {
        mport = dbMportCreate(netlist, utSymCreateFormatted("%s[%u]", utSymGetName(name), xMport),
            type);
        dbMbusInsertMport(mbus, index, mport);
        index++;
    } utWhile(xMport != right) {
        if(left < right) {
            xMport++;
        } else {
            xMport--;
        }
    } utRepeat;
}

/*--------------------------------------------------------------------------------------------------
  Construct a new mbus object.  Explode it, creating all it's mports.
--------------------------------------------------------------------------------------------------*/
dbMbus dbMbusCreate(
    dbNetlist netlist,
    utSym name,
    dbMportType type,
    uint32 left,
    uint32 right)
{
    dbMbus mbus = dbMbusAlloc();

    dbMbusSetSym(mbus, name);
    dbMbusSetLeft(mbus, left);
    dbMbusSetRight(mbus, right);
    dbNetlistInsertMbus(netlist, mbus);
    dbMbusSetType(mbus, type);
    explodeMbus(mbus, type);
    return mbus;
}

/*--------------------------------------------------------------------------------------------------
  Construct a new port object.
--------------------------------------------------------------------------------------------------*/
dbPort dbPortCreate(
    dbInst inst,
    dbMport mport)
{
    dbPort port = dbPortAlloc();

    dbInstAppendPort(inst, port);
    dbPortSetMport(port, mport);
    return port;
}

/*--------------------------------------------------------------------------------------------------
  Construct a new instance object.
--------------------------------------------------------------------------------------------------*/
dbInst dbInstCreate(
    dbNetlist netlist,
    utSym name,
    dbNetlist internalNetlist)
{
    dbInst inst;

    if(name != utSymNull) {
        inst = dbNetlistFindInst(netlist, name);
        if(inst != dbInstNull) {
            utExit("dbInstCreate: inst %s already defined", utSymGetName(name));
        }
    }
    inst = dbInstAlloc();
    dbInstSetSym(inst, name);
    dbNetlistAppendInst(netlist, inst);
    dbNetlistAppendExternalInst(internalNetlist, inst);
    return inst;
}

/*--------------------------------------------------------------------------------------------------
  Construct a new join instance.
--------------------------------------------------------------------------------------------------*/
dbInst dbJoinInstCreate(
    dbNetlist netlist)
{
    dbDesign design = dbNetlistGetDesign(netlist);
    utSym netlistName = dbDesignCreateUniqueNetlistName(design, "join");
    dbNetlist joinNetlist = dbNetlistCreate(dbNetlistGetDesign(netlist), netlistName, DB_JOIN,
        utSymNull);
    utSym instName = dbNetlistCreateUniqueInstName(netlist, "join");

    return dbInstCreate(netlist, instName, joinNetlist);
}

/*--------------------------------------------------------------------------------------------------
  Just add the net to the join.
--------------------------------------------------------------------------------------------------*/
void dbJoinInstAddNet(
    dbInst join,
    dbNet net)
{
    dbNetlist netlist = dbInstGetInternalNetlist(join);
    utSym mportName = dbNetlistCreateUniqueMportName(netlist, "join");
    dbMport joinMport = dbMportCreate(netlist, mportName, DB_PAS);
    dbPort port = dbPortCreate(join, joinMport);

    dbNetInsertPort(net, port);
}

/*--------------------------------------------------------------------------------------------------
  Join two nets together.
--------------------------------------------------------------------------------------------------*/
void dbJoinNets(
    dbNet net1,
    dbNet net2)
{
    dbInst join = dbJoinInstCreate(dbNetGetNetlist(net1));

    dbJoinInstAddNet(join, net1);
    dbJoinInstAddNet(join, net2);
}

/*--------------------------------------------------------------------------------------------------
  Construct a new instance object.  Check to see if it's global, and if so, add it to the global.
--------------------------------------------------------------------------------------------------*/
dbNet dbNetCreate(
    dbNetlist netlist,
    utSym name)
{
    dbGlobal global;
    dbNet net;

    if(name != utSymNull) {
        net = dbNetlistFindNet(netlist, name);
        if(net != dbNetNull) {
            utExit("dbNetCreate: Net %s already defined", utSymGetName(name));
        }
    }
    net = dbNetAlloc();
    dbNetSetSym(net, name);
    dbNetlistAppendNet(netlist, net);
    if(name != utSymNull) {
        global = dbDesignFindGlobal(dbNetlistGetDesign(netlist), name);
        if(global != dbGlobalNull) {
            dbGlobalAppendNet(global, net);
        }
    }
    return net;
}

/*--------------------------------------------------------------------------------------------------
  Create a net attached to a global.  Use the same name.  Return the old net, if it already
  exists.
--------------------------------------------------------------------------------------------------*/
dbNet dbGlobalNetCreate(
    dbNetlist netlist,
    dbGlobal global)
{
    utSym name = dbGlobalGetSym(global);
    dbNet net = dbNetlistFindNet(netlist, name);

    if(net != dbNetNull) {
        if(dbNetGetGlobal(net) != global) {
            utError("Net %s in netlist %s is not attached to the global version",
                utSymGetName(name), dbNetlistGetName(netlist));
        }
        return net;
    }
    return dbNetCreate(netlist, name);
}

/*--------------------------------------------------------------------------------------------------
  Create the one net for the netlist, if it does not yet exist.
--------------------------------------------------------------------------------------------------*/
dbNet dbNetlistGetOneNet(
    dbNetlist netlist)
{
    return dbGlobalNetCreate(netlist, dbDesignGetOneGlobal(dbNetlistGetDesign(netlist)));
}

/*--------------------------------------------------------------------------------------------------
  Create the zero net for the netlist, if it does not yet exist.
--------------------------------------------------------------------------------------------------*/
dbNet dbNetlistGetZeroNet(
    dbNetlist netlist)
{
    return dbGlobalNetCreate(netlist, dbDesignGetZeroGlobal(dbNetlistGetDesign(netlist)));
}

/*--------------------------------------------------------------------------------------------------
  Return the net with the given name if it exists, otherwise build it.
--------------------------------------------------------------------------------------------------*/
dbNet dbNetFindOrCreate(
    dbNetlist netlist,
    utSym name)
{
    dbNet net = dbNetlistFindNet(netlist, name);

    if(net != dbNetNull) {
        return net;
    }
    return dbNetCreate(netlist, name);
}

/*--------------------------------------------------------------------------------------------------
  Construct a new global object, or return the old one if it already exists.
--------------------------------------------------------------------------------------------------*/
dbGlobal dbGlobalCreate(
    dbDesign design,
    utSym name)
{
    dbGlobal global = dbDesignFindGlobal(design, name);

    if(global != dbGlobalNull) {
        return global;
    }
    global = dbGlobalAlloc();
    dbGlobalSetSym(global, name);
    dbDesignInsertGlobal(design, global);
    return global;
}

/*--------------------------------------------------------------------------------------------------
  Construct a new attribute object.
--------------------------------------------------------------------------------------------------*/
dbAttr dbAttrCreate(
    utSym  name,
    utSym value)
{
    dbAttr attr = dbAttrAlloc();

    dbAttrSetName(attr, name);
    dbAttrSetValue(attr, value);
    return attr;
}

/*--------------------------------------------------------------------------------------------------
  Make a copy of an attribute.
--------------------------------------------------------------------------------------------------*/
dbAttr dbAttrCopy(
    dbAttr attr)
{
    return dbAttrCreate(dbAttrGetName(attr), dbAttrGetValue(attr));
}

/*--------------------------------------------------------------------------------------------------
  Copy an attribute list.
--------------------------------------------------------------------------------------------------*/
dbAttr dbCopyAttrs(
    dbAttr attr)
{
    dbAttr newAttr;
    dbAttr childAttr;

    if(attr == dbAttrNull) {
        return dbAttrNull;
    }
    childAttr = dbCopyAttrs(dbAttrGetNextAttr(attr));
    newAttr = dbAttrCopy(attr);
    dbAttrSetNextAttr(newAttr, childAttr);
    return newAttr;
}

/*--------------------------------------------------------------------------------------------------
  Explode nets in the bus.
--------------------------------------------------------------------------------------------------*/
static void explodeBus(
    dbBus bus,
    uint32 left,
    uint32 right)
{
    dbNetlist netlist = dbBusGetNetlist(bus);
    dbNet net;
    utSym name = dbBusGetSym(bus);
    uint32 xNet = left;
    uint32 index = 0;

    dbBusAllocNets(bus, utAbs((int32)(right - left)) + 1);
    utDo {
        net = dbNetFindOrCreate(netlist, utSymCreateFormatted("%s[%u]", utSymGetName(name), xNet));
        dbBusInsertNet(bus, index, net);
        index++;
    } utWhile(xNet != right) {
        if(left < right) {
            xNet++;
        } else {
            xNet--;
        }
    } utRepeat;
}

/*--------------------------------------------------------------------------------------------------
  Create a new bus.  Explode it, creating all it's nets.
--------------------------------------------------------------------------------------------------*/
dbBus dbBusCreate(
    dbNetlist netlist,
    utSym name,
    uint32 left,
    uint32 right)
{
    dbBus bus = dbBusAlloc();

    dbBusSetSym(bus, name);
    dbNetlistInsertBus(netlist, bus);
    dbBusSetLeft(bus, left);
    dbBusSetRight(bus, right);
    explodeBus(bus, left, right);
    return bus;
}

/*--------------------------------------------------------------------------------------------------
  Create a new devspec object.  This is just a simple object to keep track of the SPICE device
  strings.
--------------------------------------------------------------------------------------------------*/
dbDevspec dbDevspecCreate(
    utSym name,
    dbSpiceTargetType type,
    char *deviceString)
{
    dbDevspec devspec = dbDevspecAlloc();

    dbDevspecSetSym(devspec, name);
    dbDevspecSetType(devspec, type);
    dbRootInsertDevspec(dbTheRoot, devspec);
    dbDevspecAllocStrings(devspec, strlen(deviceString) + 1);
    strcpy(dbDevspecGetString(devspec), deviceString);
    return devspec;
}

