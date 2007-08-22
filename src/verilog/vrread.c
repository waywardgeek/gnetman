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

/*--------------------------------------------------------------------------------------------------
   This module reads and writes gate-level Verilog designs.
--------------------------------------------------------------------------------------------------*/
#include "vr.h"

FILE *vrFile;
uint32 vrFileSize, vrLineNum, vrCharCount, vrLinePos;
dbDesign vrCurrentDesign, vrCurrentLibrary;

/*--------------------------------------------------------------------------------------------------
  Determine if a value is in the range.
--------------------------------------------------------------------------------------------------*/
static bool inRange(
    uint32 index,
    uint32 left,
    uint32 right)
{
    if(left <= right) {
        return left <= index && index <= right;
    }
    return right <= index && index <= left;
}

/*--------------------------------------------------------------------------------------------------
  Build a new netlist in the database.
--------------------------------------------------------------------------------------------------*/
dbNetlist vrNetlistCreate(
    dbDesign design,
    utSym name)
{
    dbNetlist netlist = dbDesignFindNetlist(design, name);

    if(netlist != dbNetlistNull) {
        utError("Module %s defined multiple times", utSymGetName(name));
    }
    netlist = dbNetlistCreate(design, name, DB_SUBCIRCUIT, utSymNull);
    return netlist;
}

/*--------------------------------------------------------------------------------------------------
  Count the total nets in the parameter.
--------------------------------------------------------------------------------------------------*/
static uint32 countParamNets(
    vrParam param)
{
    vrConn conn;
    uint32 numNets = 0;

    vrForeachParamConn(param, conn) {
        if(vrConnConst(conn)) {
            numNets += vrConnGetLength(conn);
        } else if(vrConnRange(conn)) {
            numNets += utAbs((int32)vrConnGetLeft(conn) - (int32)vrConnGetRight(conn)) + 1;
        } else if(vrConnGetBus(conn) != dbBusNull) {
            numNets += dbBusGetNumNet(vrConnGetBus(conn));
        } else {
            numNets++;
        }
    } vrEndParamConn;
    return numNets;
}

/*--------------------------------------------------------------------------------------------------
  Check that the connectivity of the mbus matches that of the parameter.
--------------------------------------------------------------------------------------------------*/
static void checkMbusConn(
    dbMbus mbus,
    vrParam param)
{
    vrIdec idec = vrParamGetIdec(param);

    if(countParamNets(param) != dbMbusGetNumMport(mbus)) {
        utError("Signal count mismatch instance %s of module %s, bus %s",
                utSymGetName(vrIdecGetSym(idec)), dbNetlistGetName(dbMbusGetNetlist(mbus)),
                dbMbusGetName(mbus));
    }
}

/*--------------------------------------------------------------------------------------------------
  Check that the connectivity of the mport matches that of the parameter.
--------------------------------------------------------------------------------------------------*/
static void checkMportConn(
    dbMport mport,
    vrParam param)
{
    vrIdec idec = vrParamGetIdec(param);
    dbMbus mbus;

    if(countParamNets(param) != 1) {
        mbus = dbMportGetMbus(mport);
        utError("Signal count mismatch instance %s of module %s, bus %s",
                utSymGetName(vrIdecGetSym(idec)), dbNetlistGetName(dbMbusGetNetlist(mbus)),
                dbMbusGetName(mbus));
    }
}

/*--------------------------------------------------------------------------------------------------
  Build a black-box netlist for the undefined instance type.
--------------------------------------------------------------------------------------------------*/
static dbNetlist buildBlackBoxNetlist(
    dbDesign design,
    vrIdec idec,
    utSym sym)
{
    dbNetlist netlist = vrNetlistCreate(design, sym); 
    vrParam param;
    dbMport mport;
    dbMbus mbus;
    dbBus bus;
    dbInst flag;
    dbNet net;
    dbPort port;
    utSym name;
    uint32 numConn;

    vrForeachIdecParam(idec, param) {
        name = vrParamGetSym(param);
        numConn = countParamNets(param);
        if(numConn == 1) {
            mport = dbMportCreate(netlist, name, DB_PAS); 
            dbFlagInstCreate(mport);
            port = dbMportGetFlagPort(mport);
            net = dbNetCreate(netlist, name);
            dbNetInsertPort(net, port);
        } else {
            bus = dbBusCreate(netlist, name, numConn - 1, 0);
            mbus = dbMbusCreate(netlist, name, DB_PAS, numConn - 1, 0);
            flag = dbBusFlagInstCreate(mbus);
            dbBusHookup(bus, dbInstGetFirstPort(flag));
        }
    } vrEndIdecParam;
    return netlist;
}

/*--------------------------------------------------------------------------------------------------
  Verify all modules are declared.  Create netlists for undeclared modules.
--------------------------------------------------------------------------------------------------*/
static void checkForImplicitModules(
    dbDesign design)
{
    dbNetlist netlist, internalNetlist;
    vrIdec idec;
    utSym sym;

    dbForeachDesignNetlist(vrCurrentDesign, netlist) {
        if(dbNetlistGetType(netlist) == DB_SUBCIRCUIT) {
            vrForeachNetlistIdec(netlist, idec) {
                sym = vrIdecGetInternalNetlistSym(idec);
                internalNetlist = dbDesignFindNetlist(design, sym);
                if(internalNetlist == dbNetlistNull) {
                    if(vrCurrentLibrary != dbDesignNull) {
                        internalNetlist = dbDesignFindNetlist(vrCurrentLibrary, sym);
                    }
                    if(internalNetlist == dbNetlistNull) {
                        utWarning("Module %s not defined.", utSymGetName(sym));
                        buildBlackBoxNetlist(design, idec, sym);
                    }
                }
            } vrEndNetlistIdec;
        }
    } dbEndDesignNetlist;
}

/*--------------------------------------------------------------------------------------------------
  Build an instance for the instance declaration.  Attach to nets.
--------------------------------------------------------------------------------------------------*/
static dbInst buildIdecInst(
    vrIdec idec)
{
    dbNetlist netlist = vrIdecGetNetlist(idec);
    dbNetlist internalNetlist = dbDesignFindNetlist(vrCurrentDesign,
        vrIdecGetInternalNetlistSym(idec));
    utSym name = vrIdecGetSym(idec);
    dbInst inst = dbNetlistFindInst(netlist, vrIdecGetSym(idec));
    dbMport mport;

    if(internalNetlist == dbNetlistNull) {
        if(vrCurrentLibrary != dbDesignNull) {
            internalNetlist = dbDesignFindNetlist(vrCurrentLibrary,
                vrIdecGetInternalNetlistSym(idec));
        }
        if(internalNetlist == dbNetlistNull) {
            utError("Netlist %s of instance %s was not defined",
                utSymGetName(vrIdecGetInternalNetlistSym(idec)), utSymGetName(name));
        }
    }
    if(inst != dbInstNull) {
        utError("Instance %s defined multiple times in netlist %s",
                utSymGetName(name), dbNetlistGetName(netlist));
    }
    inst = dbInstCreate(netlist, name, internalNetlist);
    dbForeachNetlistMport(internalNetlist, mport) {
        dbPortCreate(inst, mport);
    } dbEndNetlistMport;
    return inst;
}

/*--------------------------------------------------------------------------------------------------
  Tie bits of the bus to zero and one as specified by the connection.
--------------------------------------------------------------------------------------------------*/
static uint32 connectPortsToConstants(
    dbInst inst,
    dbMbus mbus,
    uint32 xMport,
    vrConn conn)
{
    dbNetlist netlist = dbInstGetNetlist(inst);
    dbMport mport;
    dbPort port;
    dbNet net;
    uint32 length = vrConnGetLength(conn);
    uint32 mask = vrConnGetMask(conn);

    while(length--) {
        if(mask & (1 << length)) {
            net = dbNetlistGetOneNet(netlist);
        } else {
            net = dbNetlistGetZeroNet(netlist);
        }
        mport = dbMbusGetiMport(mbus, xMport);
        port = dbFindPortFromInstMport(inst, mport);
        dbNetAppendPort(net, port);
        xMport++;
    }
    return xMport;
}

/*--------------------------------------------------------------------------------------------------
  Tie bits of the bus to a range of the bus specified by the conn.
--------------------------------------------------------------------------------------------------*/
static uint32 connectPortsToRange(
    dbInst inst,
    dbMbus mbus,
    uint32 xMport,
    vrConn conn)
{
    dbMport mport;
    dbPort port;
    dbNet net;
    dbBus bus = vrConnGetBus(conn);
    int32 left, right;
    int32 xNet;

    if(vrConnRange(conn)) {
        left = vrConnGetLeft(conn);
        right = vrConnGetRight(conn);
        if(!inRange(left,  dbBusGetLeft(bus), dbBusGetRight(bus)) ||
            !inRange(right, dbBusGetLeft(bus), dbBusGetRight(bus))) {
            utError("Invalid bus index for bus %s on instance %s",
                    dbBusGetName(bus), dbInstGetUserName(inst));
        }
    } else {
        left = dbBusGetLeft(bus);
        right = dbBusGetRight(bus);
    }
    for(xNet = left; left <= right? xNet <= right: xNet >= right; left <= right? xNet++ : xNet--) {
        mport = dbMbusGetiMport(mbus, xMport);
        port = dbFindPortFromInstMport(inst, mport);
        net = dbBusIndexNet(bus, xNet);
        dbNetAppendPort(net, port);
        xMport++;
    }
    return xMport;
}

/*--------------------------------------------------------------------------------------------------
  Tie a bit of a bus to a bit of the bus specified by the conn.
--------------------------------------------------------------------------------------------------*/
static void connectPortToNet(
    dbInst inst,
    dbMbus mbus,
    uint32 xMport,
    vrConn conn)
{
    dbMport mport = dbMbusGetiMport(mbus, xMport);
    dbPort port = dbFindPortFromInstMport(inst, mport);
    dbNet net = vrConnGetNet(conn);

    dbNetAppendPort(net, port);
}

/*--------------------------------------------------------------------------------------------------
  Add a connection from a bus to nets specified by the parameter.
--------------------------------------------------------------------------------------------------*/
static void addBusConnection(
    dbInst inst,
    vrParam param,
    dbMbus mbus)
{
    vrConn conn;
    uint32 xMport = 0;

    vrForeachParamConn(param, conn) {
        if(xMport > dbMbusGetNumMport(mbus)) {
            utError("Mismatched signal width on port %s of inst %s",
                utSymGetName(vrParamGetSym(param)), dbInstGetUserName(inst));
        }
        if(vrConnConst(conn)) {
            xMport = connectPortsToConstants(inst, mbus, xMport, conn);
        } else if(vrConnGetBus(conn) != dbBusNull) {
            xMport = connectPortsToRange(inst, mbus, xMport, conn);
        } else {
            connectPortToNet(inst, mbus, xMport, conn);
            xMport++;
        }
    } vrEndParamConn;
    if(xMport != dbMbusGetNumMport(mbus)) {
        utError("Mismatched signal width on port %s of inst %s",
            utSymGetName(vrParamGetSym(param)), dbInstGetUserName(inst));
    }
}

/*--------------------------------------------------------------------------------------------------
  Connect a net to the port as specified by the connection.
--------------------------------------------------------------------------------------------------*/
static void addNetConnection(
    dbInst inst,
    vrConn conn,
    dbMport mport)
{
    dbNetlist netlist = dbInstGetNetlist(inst);
    dbPort port = dbFindPortFromInstMport(inst, mport);
    dbNet net;
    dbBus bus;
    utSym sym;

    if(conn == vrConnNull) {
        sym = dbNetlistCreateUniqueNetName(netlist, "N");
        net = dbNetCreate(netlist, sym);
    } else if(vrConnConst(conn)) {
        if(vrConnGetMask(conn)) {
            net = dbNetlistGetOneNet(netlist);
        } else {
            net = dbNetlistGetZeroNet(netlist);
        }
    } else if(vrConnRange(conn) && vrConnGetLeft(conn) == vrConnGetRight(conn)) {
        bus = vrConnGetBus(conn);
        net = dbBusIndexNet(bus, vrConnGetLeft(conn));
    } else {
        if(vrConnGetBus(conn) != dbBusNull) {
            utError("Bus connection to %s on inst %s passed to scalar signal",
                dbBusGetName(vrConnGetBus(conn)), dbInstGetUserName(inst));
        }
        net = vrConnGetNet(conn);
        utAssert(net != dbNetNull);
    }
    dbNetAppendPort(net, port);
}

/*--------------------------------------------------------------------------------------------------
  Skip over mports until we reach an new mbus, or an mport with no owning mbus.
--------------------------------------------------------------------------------------------------*/
static dbMport skipParam(
    dbMport mport)
{
    dbMbus mbus = dbMportGetMbus(mport);

    do {
        mport = dbMportGetNextNetlistMport(mport);
    } while(mport != dbMportNull && mbus != dbMbusNull && dbMportGetMbus(mport) == mbus);
    return mport;
}

/*--------------------------------------------------------------------------------------------------
  Find the nth parameter on an instance.
--------------------------------------------------------------------------------------------------*/
static utSym findPositionalParamSym(
    dbInst inst,
    uint32 paramNum)
{
    dbNetlist internalNetlist = dbInstGetInternalNetlist(inst);
    dbMbus mbus;
    dbMport mport;
    uint32 xParam;

    utAssert(internalNetlist != dbNetlistNull);
    mport = dbNetlistGetFirstMport(internalNetlist);
    for(xParam = 1; xParam < paramNum && mport != dbMportNull; xParam++) {
        mport = skipParam(mport);
    }
    if(mport == dbMportNull) {
        utError("Too many parameters passed to inst %s", dbInstGetUserName(inst));
    }
    mbus = dbMportGetMbus(mport);
    if(mbus != dbMbusNull) {
        return dbMbusGetSym(mbus);
    }
    return dbMportGetSym(mport);
}

/*--------------------------------------------------------------------------------------------------
  Connect nets to ports of the instance as specified by the instance declaration.
--------------------------------------------------------------------------------------------------*/
static void addNetPorts(
    vrIdec idec,
    dbInst inst)
{
    dbNetlist internalNetlist = dbInstGetInternalNetlist(inst);
    vrParam param;
    dbMbus mbus;
    dbMport mport;
    utSym paramSym;
    uint32 paramNum = 1;

    vrForeachIdecParam(idec, param) {
        paramSym = vrParamGetSym(param);
        if(paramSym == utSymNull) {
            paramSym = findPositionalParamSym(inst, paramNum);
        }
        mbus = dbNetlistFindMbus(internalNetlist, paramSym);
        if(mbus != dbMbusNull) {
            addBusConnection(inst, param, mbus);
        } else {
            mport = dbNetlistFindMport(internalNetlist, paramSym);
            if(mport == dbMportNull) {
                utError("Pin %s of instance %s does not exist on cell %s",
                        utSymGetName(paramSym), dbInstGetUserName(inst),
                        dbNetlistGetName(internalNetlist));
            }
            if(vrParamGetFirstConn(param) != vrConnNull) {
                utAssert(vrConnGetNextParamConn(vrParamGetFirstConn(param)) == vrConnNull);
            }
            addNetConnection(inst, vrParamGetFirstConn(param), mport);
        }
        paramNum++;
    } vrEndIdecParam;
}

/*--------------------------------------------------------------------------------------------------
  Convert the instance declaration objects into instances.
--------------------------------------------------------------------------------------------------*/
static void buildInstances(void)
{
    dbNetlist netlist;
    vrIdec idec;
    dbInst inst;

    dbForeachDesignNetlist(vrCurrentDesign, netlist) {
        if(dbNetlistGetType(netlist) == DB_SUBCIRCUIT) {
            vrForeachNetlistIdec(netlist, idec) {
                inst = buildIdecInst(idec);
                addNetPorts(idec, inst);
            } vrEndNetlistIdec;
        }
    } dbEndDesignNetlist;
}

/*--------------------------------------------------------------------------------------------------
  Find the instance refered to by the path.
--------------------------------------------------------------------------------------------------*/
static dbInst findInstFromPath(
    dbNetlist netlist,
    vrPath path)
{
    vrPath nextPath = vrPathGetNextDefparamPath(path);
    dbInst inst = dbNetlistFindInst(netlist, vrPathGetSym(path));

    if(inst == dbInstNull) {
        /* Invalid path */
        utWarning("defparam statement failed to find instance");
        return dbInstNull;
    }
    if(nextPath != vrPathNull) {
        if(dbInstGetType(inst) != DB_SUBCIRCUIT) {
            utWarning("defparam specifies instance within leaf instance");
            return dbInstNull;
        }
        return findInstFromPath(dbInstGetInternalNetlist(inst), nextPath);
    }
    return inst;
}

/*--------------------------------------------------------------------------------------------------
  Create properties on instances from defparam statements.  This has to be
  done as a post process, since parameters can be assigned before an
  instance is declared.
--------------------------------------------------------------------------------------------------*/
static void evaluateDefparams(void)
{
    dbNetlist netlist;
    vrDefparam defparam;
    dbInst inst;

    dbForeachDesignNetlist(vrCurrentDesign, netlist) {
        if(dbNetlistGetType(netlist) == DB_SUBCIRCUIT) {
            vrForeachNetlistDefparam(netlist, defparam) {
                netlist = vrDefparamGetNetlist(defparam);
                inst = findInstFromPath(netlist, vrDefparamGetFirstPath(defparam));
                if(inst != dbInstNull) {
                    dbInstSetValue(inst, vrDefparamGetSym(defparam), vrDefparamGetValue(defparam));
                }
            } vrEndNetlistDefparam;
        }
    } dbEndDesignNetlist;
}

/*--------------------------------------------------------------------------------------------------
  Initialize the verilog reader module.
--------------------------------------------------------------------------------------------------*/
void vrInit(void) 
{
    vrDatabaseStart();
}

/*--------------------------------------------------------------------------------------------------
  Free memory used by the verilog reader.
--------------------------------------------------------------------------------------------------*/
void vrClose(void)
{
    vrDatabaseStop();
}

/*--------------------------------------------------------------------------------------------------
  Warn if nets and insts have the same names.
--------------------------------------------------------------------------------------------------*/
static bool checkNamesInNetlist(
    dbNetlist netlist)
{
    dbNet net;
    dbInst inst;
    dbMport mport;
    utSym sym;

    dbForeachNetlistNet(netlist, net) {
        sym = dbNetGetSym(net);
        inst = dbNetlistFindInst(netlist, sym);
        if(inst != dbInstNull) {
            utWarning("Inst %s and net %s have the same name.",
                dbInstGetUserName(inst), dbNetGetName(net));
            return false;
        }
    } dbEndNetlistNet;
    dbForeachNetlistMport(netlist, mport) {
        sym = dbMportGetSym(mport);
        inst = dbNetlistFindInst(netlist, sym);
        if(inst != dbInstNull) {
            utWarning("Port %s and inst %s have the same name.",
                dbMportGetName(mport), dbInstGetUserName(inst));
            return false;
        }
    } dbEndNetlistMport;
    return true;
}

/*--------------------------------------------------------------------------------------------------
  Warn if nets and insts have the same names.
--------------------------------------------------------------------------------------------------*/
void vrCheckNamesInDesign(
    dbDesign design)
{
    dbNetlist netlist;

    dbForeachDesignNetlist(design, netlist) {
        if(dbNetlistGetType(netlist) == DB_SUBCIRCUIT) {
            if(!checkNamesInNetlist(netlist)) {
                return;
            }
        }
    } dbEndDesignNetlist;
}

/*--------------------------------------------------------------------------------------------------
  Read a Verilog gate-level design into the database.
--------------------------------------------------------------------------------------------------*/
dbDesign vrReadDesign(
    char *designName,
    char *fileName,
    dbDesign libDesign)
{
    utSym name = utSymCreate(designName);

    utLogMessage("Reading Verilog file %s", fileName);
    vrFileSize = utFindFileSize(fileName);
    vrFile = fopen(fileName, "r");
    if(!vrFile) {
        utWarning("Could not open file %s for reading", fileName);
        return dbDesignNull;
    }
    vrCharCount = 0;
    vrLineNum = 1;
    vrCurrentLibrary = libDesign;
    vrCurrentDesign = dbRootFindDesign(dbTheRoot, name);
    if(vrCurrentDesign == dbDesignNull) {
        vrCurrentDesign = dbDesignCreate(name, libDesign);
    }
    vrInit();
    if(vrparse()) {
        fclose(vrFile);
        vrClose();
        dbDesignDestroy(vrCurrentDesign);
        return dbDesignNull;
    }
    fclose(vrFile);
    checkForImplicitModules(vrCurrentDesign);
    buildInstances();
    evaluateDefparams();
    vrCheckNamesInDesign(vrCurrentDesign);
    dbDesignBuildNetsForFloatingPorts(vrCurrentDesign);
    vrClose();
    return vrCurrentDesign;
}

