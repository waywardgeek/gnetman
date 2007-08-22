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
  Basic netlist manipulations.
--------------------------------------------------------------------------------------------------*/
#include <string.h>
#include <ctype.h>
#include "db.h"

/*--------------------------------------------------------------------------------------------------
  Rename the net.
--------------------------------------------------------------------------------------------------*/
void dbNetRename(
    dbNet net,
    utSym newName)
{
    dbNetlist netlist = dbNetGetNetlist(net);
    dbNet otherNet = dbNetlistFindNet(netlist, newName);

    if(otherNet != dbNetNull) {
        utExit("Net %s cannot be renamed to %s, since that net already exists in netlist %s",
            dbNetGetName(net), utSymGetName(newName), dbNetlistGetName(netlist));
    }
    dbNetlistRemoveNet(netlist, net);
    dbNetSetSym(net, newName);
    dbNetlistInsertNet(netlist, net);
}

/*--------------------------------------------------------------------------------------------------
  Rename the instance.
--------------------------------------------------------------------------------------------------*/
void dbInstRename(
    dbInst inst,
    utSym newName)
{
    dbNetlist netlist = dbInstGetNetlist(inst);
    dbInst otherInst = dbNetlistFindInst(netlist, newName);

    if(otherInst != dbInstNull) {
        utExit("Instance %s cannot be renamed to %s, since that instance already exists in "
            "netlist %s", dbInstGetUserName(inst), utSymGetName(newName), dbNetlistGetName(netlist));
    }
    dbNetlistRemoveInst(netlist, inst);
    dbInstSetSym(inst, newName);
    dbNetlistInsertInst(netlist, inst);
}

/*--------------------------------------------------------------------------------------------------
  Rename the global.  Also rename all it's nets.
--------------------------------------------------------------------------------------------------*/
void dbGlobalRename(
    dbGlobal global,
    utSym newName)
{
    dbDesign design = dbGlobalGetDesign(global);
    dbNet net;

    dbDesignRemoveGlobal(design, global);
    dbGlobalSetSym(global, newName);
    dbDesignInsertGlobal(design, global);
    dbForeachGlobalNet(global, net) {
        dbNetRename(net, newName);
    } dbEndGlobalNet;
}

/*--------------------------------------------------------------------------------------------------
  Rename the netlist.  Also rename all it's nets.
--------------------------------------------------------------------------------------------------*/
void dbNetlistRename(
    dbNetlist netlist,
    utSym newName)
{
    dbDesign design = dbNetlistGetDesign(netlist);

    dbDesignRemoveNetlist(design, netlist);
    dbNetlistSetSym(netlist, newName);
    dbDesignInsertNetlist(design, netlist);
}

/*--------------------------------------------------------------------------------------------------
  Override interal net names, setting net names to match mport names.
--------------------------------------------------------------------------------------------------*/
void dbNetlistSetNetNamesToMatchMports(
    dbNetlist netlist)
{
    dbNet net, otherNet;
    dbMport mport;
    dbPort port;
    utSym name, otherName;

    dbNetlistClearNetVisitedFlags(netlist);
    dbForeachNetlistMport(netlist, mport) {
        port = dbMportGetFlagPort(mport);
        if(port != dbPortNull) {
            net = dbPortGetNet(port);
            if(net != dbNetNull) {
                if(dbNetVisited(net)) {
                    utWarning("Net %s connects to multiple mports in netlist %s",
                        dbNetGetName(net), dbNetlistGetName(netlist));
                } else {
                    dbNetSetVisited(net, true);
                    name = dbMportGetSym(mport);
                    if(name != dbNetGetSym(net)) {
                        otherNet = dbNetlistFindNet(netlist, name);
                        if(otherNet != dbNetNull) {
                            otherName = dbNetlistCreateUniqueNetName(netlist, utSymGetName(name));
                            dbNetRename(otherNet, otherName);
                        }
                        dbNetRename(net, name);
                    }
                }
            }
        }
    } dbEndNetlistMport;
}

/*--------------------------------------------------------------------------------------------------
  Override interal net names, setting net names to match mport names.
--------------------------------------------------------------------------------------------------*/
void dbDesignSetNetNamesToMatchMports(
    dbDesign design)
{
    dbNetlist netlist;

    dbForeachDesignNetlist(design, netlist) {
        dbNetlistSetNetNamesToMatchMports(netlist);
    } dbEndDesignNetlist;
}

/*--------------------------------------------------------------------------------------------------
  Set netlist names to upper case.  Issue warnings if there are name collisions.
--------------------------------------------------------------------------------------------------*/
void dbDesignMakeNetlistNamesUpperCase(
    dbDesign design)
{
    dbNetlist netlist;
    utSym newName;

    dbSafeForeachDesignNetlist(design, netlist) {
        newName = utSymCreate(utStringToUpperCase(dbNetlistGetName(netlist)));
        if(newName != dbNetlistGetSym(netlist)) {
            dbNetlistRename(netlist, newName);
        }
    } dbEndSafeForeachDesignNetlist;
}

/*--------------------------------------------------------------------------------------------------
  Convert power instances into global net objects.
--------------------------------------------------------------------------------------------------*/
void dbNetlistConvertPowerInstsToGlobals(
    dbNetlist netlist)
{
    dbDesign design = dbNetlistGetDesign(netlist);
    dbNetlist internalNetlist;
    dbGlobal global;
    dbMport mport;
    dbInst inst;
    dbNet net, globalNet;
    utSym globalSym;

    dbSafeForeachNetlistInst(netlist, inst) {
        internalNetlist = dbInstGetInternalNetlist(inst);
        if(dbNetlistGetType(internalNetlist) == DB_POWER) {
            mport = dbNetlistGetFirstMport(internalNetlist);
            if(mport != dbMportNull) {
                net = dbPortGetNet(dbInstGetFirstPort(inst));
                globalSym = dbMportGetSym(mport);
                global = dbGlobalCreate(design, globalSym);
                if(dbNetGetGlobal(net) == dbGlobalNull) {
                    globalNet = dbGlobalNetCreate(netlist, global);
                    dbMergeNetIntoNet(net, globalNet);
                } else if(dbNetGetGlobal(net) != global) {
                    utWarning("Net %s in netlist %s assigned to global nets %s and %s",
                        dbNetGetName(net), dbNetlistGetName(netlist),
                        dbGlobalGetName(global), dbGlobalGetName(dbNetGetGlobal(net)));
                }
                dbInstDestroy(inst);
            }
        }
    } dbEndSafeForeachNetlistInst;
}

/*--------------------------------------------------------------------------------------------------
  Convert power instances into global net objects.
--------------------------------------------------------------------------------------------------*/
void dbDesignConvertPowerInstsToGlobals(
    dbDesign design)
{
    dbNetlist netlist;

    dbForeachDesignNetlist(design, netlist) {
        dbNetlistConvertPowerInstsToGlobals(netlist);
    } dbEndDesignNetlist;
}

/*--------------------------------------------------------------------------------------------------
  Find the name for one bit of an array of instances.
--------------------------------------------------------------------------------------------------*/
static utSym findArrayInstName(
    dbInst arrayInst,
    uint32 xInst)
{
    char *buf = utCopyString(dbInstGetUserName(arrayInst));
    char *p = strrchr(buf, '[');

    utAssert(p != NULL);
    *p = '\0';
    return utSymCreateFormatted("%s[%u]", buf, xInst);
}

/*--------------------------------------------------------------------------------------------------
  Determine if the port belongs to a bus mport.
--------------------------------------------------------------------------------------------------*/
static bool portIsBus(
    dbPort port)
{
    dbMport mport = dbPortGetMport(port);
    dbPort nextPort = dbPortGetNextInstPort(port);

    if(nextPort != dbPortNull) {
        return dbPortGetMport(nextPort) == mport;
    }
    return false;
}

/*--------------------------------------------------------------------------------------------------
  Skip ports on the instance so we can get to a particular bit in a bus.
--------------------------------------------------------------------------------------------------*/
static dbPort skipPorts(
    dbPort port,
    uint32 portPosition)
{
    uint32 xPort;

    for(xPort = 0; xPort < portPosition; xPort++) {
        port = dbPortGetNextInstPort(port);
    }
    return port;
}

/*--------------------------------------------------------------------------------------------------
  Skip past ports belonging to the same mbus.
--------------------------------------------------------------------------------------------------*/
static dbPort skipPortPastMbus(
    dbPort port,
    dbMbus mbus)
{
    while(port != dbPortNull && dbMportGetMbus(dbPortGetMport(port)) == mbus) {
        port = dbPortGetNextInstPort(port);
    }
    return port;
}

/*--------------------------------------------------------------------------------------------------
  Find the width of a port.  Just find out how many ports from this one have the same owning
  mbus.
--------------------------------------------------------------------------------------------------*/
static uint32 findPortWidth(
    dbPort port)
{
    dbMbus mbus = dbMportGetMbus(dbPortGetMport(port));
    uint32 width = 0;

    do {
        width++;
        port = dbPortGetNextInstPort(port);
    } while(port != dbPortNull && dbMportGetMbus(dbPortGetMport(port)) == mbus);
    return width;
}

/*--------------------------------------------------------------------------------------------------
  Build the ports on the new bit instance of an instance array.
--------------------------------------------------------------------------------------------------*/
static void buildArrayInstPorts(
    dbInst arrayInst,
    dbInst inst,
    uint32 portPosition,
    uint32 instWidth)
{
    dbPort oldPort = dbInstGetFirstPort(arrayInst);
    dbPort newPort;
    dbMbus mbus;
    dbMport mport;
    uint32 xPort, busWidth;
    bool busExploded;

    while(oldPort != dbPortNull) {
        mport = dbPortGetMport(oldPort);
        mbus = dbMportGetMbus(mport);
        if(mbus == dbMbusNull && portIsBus(oldPort)) {
            oldPort = skipPorts(oldPort, portPosition);
            newPort = dbPortCreate(inst, mport);
            dbNetInsertPort(dbPortGetNet(oldPort), newPort);
            oldPort = skipPorts(oldPort, instWidth - portPosition);
        } else if(mbus != dbMbusNull) {
            busWidth = utAbs((int32)dbMbusGetRight(mbus) - (int32)dbMbusGetLeft(mbus)) + 1;
            busExploded = findPortWidth(oldPort) > busWidth;
            if(busExploded) {
                oldPort = skipPorts(oldPort, portPosition*busWidth);
            }
            for(xPort = 0; xPort < busWidth; xPort++) {
                mport = dbPortGetMport(oldPort);
                newPort = dbPortCreate(inst, mport);
                dbNetInsertPort(dbPortGetNet(oldPort), newPort);
                oldPort = dbPortGetNextInstPort(oldPort);
            }
            if(busExploded) {
                oldPort = skipPortPastMbus(oldPort, mbus);
            }
        } else {
            newPort = dbPortCreate(inst, mport);
            dbNetInsertPort(dbPortGetNet(oldPort), newPort);
            oldPort = dbPortGetNextInstPort(oldPort);
        }
    }
}

/*--------------------------------------------------------------------------------------------------
  Build one instance of an array of instances.
--------------------------------------------------------------------------------------------------*/
static dbInst buildOneInstOfArray(
    dbInst arrayInst,
    uint32 xInst,
    uint32 portPosition,
    uint32 instWidth)
{
    dbNetlist internalNetlist = dbInstGetInternalNetlist(arrayInst);
    utSym name = findArrayInstName(arrayInst, xInst);
    dbInst inst = dbInstCreate(dbInstGetNetlist(arrayInst), name, internalNetlist);

    dbInstSetAttr(inst, dbCopyAttrs(dbInstGetAttr(arrayInst)));
    buildArrayInstPorts(arrayInst, inst, portPosition, instWidth);
    return inst;
}

/*--------------------------------------------------------------------------------------------------
  Explode an array instance into individual non-array instances.  Note that this destroys the inst.
--------------------------------------------------------------------------------------------------*/
void dbInstExplode(
    dbInst inst)
{
    uint32 left, right;
    bool isArray = dbNameHasRange(dbInstGetUserName(inst), &left, &right);
    uint32 xInst = left;
    uint32 portPosition = 0;

    utAssert(isArray);
    utDo {
        buildOneInstOfArray(inst, xInst, portPosition, utAbs((int32)right - (int32)left) + 1);
    } utWhile(xInst != right) {
        if(xInst < right) {
            xInst++;
        } else {
            xInst--;
        }
        portPosition++;
    } utRepeat;
    dbInstDestroy(inst);
}

/*--------------------------------------------------------------------------------------------------
  Explode instance arrays in the netlist into individual non-array instances.
--------------------------------------------------------------------------------------------------*/
void dbNetlistExplodeArrayInsts(
    dbNetlist netlist)
{
    dbInst inst;

    dbSafeForeachNetlistInst(netlist, inst) {
        if(dbInstArray(inst) && dbInstGetType(inst) != DB_FLAG) {
            dbInstExplode(inst);
        }
    } dbEndSafeForeachNetlistInst;
}

/*--------------------------------------------------------------------------------------------------
  Explode instance arrays in the design into individual non-array instances.
--------------------------------------------------------------------------------------------------*/
void dbDesignExplodeArrayInsts(
    dbDesign design)
{
    dbNetlist netlist;

    dbForeachDesignNetlist(design, netlist) {
        dbNetlistExplodeArrayInsts(netlist);
    } dbEndDesignNetlist;
}

/*--------------------------------------------------------------------------------------------------
  Determine if the name contains only letters, digits, and '_'.
--------------------------------------------------------------------------------------------------*/
static bool nameContainsNonAlnumChars(
    char *name)
{
    while(isalnum(*name) || *name == '_') {
        name++;
    }
    return *name != '\0';
}

/*--------------------------------------------------------------------------------------------------
  Replace all non-alphanumeric characters with '_', except for any trailing non-alnum char.
--------------------------------------------------------------------------------------------------*/
static char *mungeName(
    char *name)
{
    char *newName = utMakeString(strlen(name) + 42); /* Note: hard-wired 42 is bigger than needed */
    char *p = newName;
    char c;
    
    utDo {
        c = *name++;
    } utWhile(c != '\0') {
        if(isalnum(c) || c == '_') {
            *p++ = c;
        } else if(*name != '\0') {
            *p++ = '_'; /* Just leave off last non-alnum char */
        }
    } utRepeat;
    *p = '\0';
    return newName;
}

/*--------------------------------------------------------------------------------------------------
  Munge an instance name.
--------------------------------------------------------------------------------------------------*/
static void mungeInstName(
    dbInst inst)
{
    dbNetlist netlist = dbInstGetNetlist(inst);
    char *name = dbInstGetName(inst);
    char *newName = mungeName(name);
    utSym sym;

    dbNetlistRenameInst(netlist, inst, utSymNull);
    sym = dbNetlistCreateUniqueInstName(netlist, newName);
    dbNetlistRenameInst(netlist, inst, sym);
}

/*--------------------------------------------------------------------------------------------------
  Convert entry names in the hash table to contain only alpha-numeric letters, and '_'.
--------------------------------------------------------------------------------------------------*/
static void mungeInstNames(
    dbNetlist netlist)
{
    dbInst inst;

    dbForeachNetlistInst(netlist, inst) {
        if(dbInstGetType(inst) !=  DB_FLAG) {
            mungeInstName(inst);
        }
    } dbEndNetlistInst;
}

/*--------------------------------------------------------------------------------------------------
  Munge an net name.
--------------------------------------------------------------------------------------------------*/
static void mungeNetName(
    dbNet net)
{
    dbNetlist netlist = dbNetGetNetlist(net);
    char *name = dbNetGetName(net);
    char *newName = mungeName(name);
    utSym sym;

    dbNetlistRenameNet(netlist, net, utSymNull);
    sym = dbNetlistCreateUniqueNetName(netlist, newName);
    dbNetlistRenameNet(netlist, net, sym);
}

/*--------------------------------------------------------------------------------------------------
  Convert entry names in the hash table to contain only alpha-numeric letters, and '_'.
--------------------------------------------------------------------------------------------------*/
static void mungeNetNames(
    dbNetlist netlist)
{
    dbNet net;

    dbForeachNetlistNet(netlist, net) {
        mungeNetName(net);
    } dbEndNetlistNet;
}

/*--------------------------------------------------------------------------------------------------
  Munge an mport name.
--------------------------------------------------------------------------------------------------*/
static void mungeMportName(
    dbMport mport)
{
    dbNetlist netlist = dbMportGetNetlist(mport);
    char *name = dbMportGetName(mport);
    char *newName = mungeName(name);
    utSym sym;

    dbNetlistRenameMport(netlist, mport, utSymNull);
    sym = dbNetlistCreateUniqueMportName(netlist, newName);
    dbNetlistRenameMport(netlist, mport, sym);
}

/*--------------------------------------------------------------------------------------------------
  Convert entry names in the hash table to contain only alpha-numeric letters, and '_'.
--------------------------------------------------------------------------------------------------*/
static void mungeMportNames(
    dbNetlist netlist)
{
    dbMport mport;

    dbForeachNetlistMport(netlist, mport) {
        mungeMportName(mport);
    } dbEndNetlistMport;
}

/*--------------------------------------------------------------------------------------------------
  Eliminate use of non alpha-numeric characters from names in the netlist, except for '_'.
  Note: This pretty much hoses mbusses and busses.  The netlist needs to be viewed bit-wise
  from this point on.
--------------------------------------------------------------------------------------------------*/
void dbNetlistEliminateNonAlnumChars(
    dbNetlist netlist)
{
    mungeInstNames(netlist);
    mungeNetNames(netlist);
    mungeMportNames(netlist);
}

/*--------------------------------------------------------------------------------------------------
  Munge an netlist name.
--------------------------------------------------------------------------------------------------*/
static void mungeNetlistName(
    dbNetlist netlist)
{
    dbDesign design = dbNetlistGetDesign(netlist);
    char *name = dbNetlistGetName(netlist);
    char *newName = mungeName(name);
    utSym sym;

    dbDesignRenameNetlist(design, netlist, utSymNull);
    sym = dbDesignCreateUniqueNetlistName(design, newName);
    dbDesignRenameNetlist(design, netlist, sym);
}

/*--------------------------------------------------------------------------------------------------
  Convert entry names in the hash table to contain only alpha-numeric letters, and '_'.
--------------------------------------------------------------------------------------------------*/
static void mungeNetlistNames(
    dbDesign design)
{
    dbNetlist netlist;

    dbForeachDesignNetlist(design, netlist) {
        mungeNetlistName(netlist);
    } dbEndDesignNetlist;
}

/*--------------------------------------------------------------------------------------------------
  Munge an global name.
--------------------------------------------------------------------------------------------------*/
static void mungeGlobalName(
    dbGlobal global)
{
    dbDesign design = dbGlobalGetDesign(global);
    char *name = dbGlobalGetName(global);
    char *newName = mungeName(name);
    utSym sym;

    dbDesignRenameGlobal(design, global, utSymNull);
    sym = dbDesignCreateUniqueGlobalName(design, newName);
    dbDesignRenameGlobal(design, global, sym);
}

/*--------------------------------------------------------------------------------------------------
  Convert entry names in the hash table to contain only alpha-numeric letters, and '_'.
--------------------------------------------------------------------------------------------------*/
static void mungeGlobalNames(
    dbDesign design)
{
    dbGlobal global;

    dbForeachDesignGlobal(design, global) {
        mungeGlobalName(global);
    } dbEndDesignGlobal;
}

/*--------------------------------------------------------------------------------------------------
  Eliminate use of non alpha-numeric characters from names in the design, except for '_'.
--------------------------------------------------------------------------------------------------*/
void dbDesignEliminateNonAlnumChars(
    dbDesign design)
{
    dbNetlist netlist;

    mungeNetlistNames(design);
    mungeGlobalNames(design);
    dbForeachDesignNetlist(design, netlist) {
        if(dbNetlistGetType(netlist) != DB_DEVICE) {
            dbNetlistEliminateNonAlnumChars(netlist);
        }
    } dbEndDesignNetlist;
}

/*--------------------------------------------------------------------------------------------------
  Build nets for floating ports in the netlist.
--------------------------------------------------------------------------------------------------*/
void dbNetlistBuildNetsForFloatingPorts(
    dbNetlist netlist)
{
    dbInst inst;
    dbNet net;
    dbPort port;
    utSym name;

    dbForeachNetlistInst(netlist, inst) {
        dbForeachInstPort(inst, port) {
            net = dbPortGetNet(port);
            if(net == dbNetNull) {
                name = dbNetlistCreateUniqueNetName(netlist, "N");
                net = dbNetCreate(netlist, name);
                dbNetInsertPort(net, port);
            }
        } dbEndInstPort;
    } dbEndNetlistInst;
}

/*--------------------------------------------------------------------------------------------------
  Build nets for all floating ports in the design.
--------------------------------------------------------------------------------------------------*/
void dbDesignBuildNetsForFloatingPorts(
    dbDesign design)
{
    dbNetlist netlist;

    dbForeachDesignNetlist(design, netlist) {
        if(dbNetlistGetType(netlist) != DB_DEVICE) {
            dbNetlistBuildNetsForFloatingPorts(netlist);
        }
    } dbEndDesignNetlist;
}

/*--------------------------------------------------------------------------------------------------
  Replace the internal netlist of an inst.  Port names must match exactly.
--------------------------------------------------------------------------------------------------*/
void dbInstReplaceInternalNetlist(
    dbInst inst,
    dbNetlist newInternalNetlist)
{
    dbNetlist oldInternalNetlist = dbInstGetInternalNetlist(inst);
    dbMport oldMport, newMport;
    dbPort port;

    dbNetlistRemoveExternalInst(oldInternalNetlist, inst);
    dbNetlistInsertExternalInst(newInternalNetlist, inst);
    dbForeachInstPort(inst, port) {
        oldMport = dbPortGetMport(port);
        newMport = dbNetlistFindMport(newInternalNetlist, dbMportGetSym(oldMport));
        if(newMport == dbMportNull) {
            utExit("Mport not found on new netlist");
        }
        dbPortSetMport(port, newMport);
    } dbEndInstPort;
}

/*--------------------------------------------------------------------------------------------------
  Connect a bus to a bus port.
--------------------------------------------------------------------------------------------------*/
void dbBusHookup(
    dbBus bus,
    dbPort firstPort)
{
    dbNet net;
    dbPort port = firstPort;

    dbForeachBusNet(bus, net) {
        if(port == dbPortNull) {
            utExit("Not enough ports to hook up bus");
        }
        dbNetInsertPort(net, port);
        port = dbPortGetNextInstPort(port);
    } dbEndBusNet;
}

static void threadGlobalNet(dbNet net, bool createTopLevelPort);
/*--------------------------------------------------------------------------------------------------
  Convert the existing external nets on instances of this mport to the global.
--------------------------------------------------------------------------------------------------*/
static void convertExternalNetsToGlobal(
    dbMport globalMport,
    dbGlobal global,
    bool createTopLevelPorts)
{
    dbNetlist netlist = dbMportGetNetlist(globalMport);
    dbNetlist extNetlist;
    dbInst extInst;
    dbPort port;
    dbNet net, globalNet;
    utSym globalSym = dbGlobalGetSym(global);
    bool needToThreadGlobal;

    dbForeachNetlistExternalInst(netlist, extInst) {
        extNetlist = dbInstGetNetlist(extInst);
        if(createTopLevelPorts || dbNetlistVisited(extNetlist)) {
            port = dbFindPortFromInstMport(extInst, globalMport);
            globalNet = dbNetlistFindNet(extNetlist, globalSym);
            needToThreadGlobal = false;
            if(globalNet == dbNetNull) {
                globalNet = dbGlobalNetCreate(extNetlist, global);
                needToThreadGlobal = true;
            }
            net = dbPortGetNet(port);
            if(net != globalNet) {
                if(dbNetGetGlobal(net) != dbGlobalNull) {
                    utWarning("Net %s assigned to global nets %s and %s", dbNetGetName(net),
                        dbGlobalGetName(global), dbGlobalGetName(dbNetGetGlobal(net)));
                }
                dbMergeNetIntoNet(net, globalNet);
            }
            if(needToThreadGlobal) {
                threadGlobalNet(globalNet, createTopLevelPorts);
            }
        }
    } dbEndNetlistExternalInst;
}

/*--------------------------------------------------------------------------------------------------
  Create external nets for this global mport signal and thread it out.
--------------------------------------------------------------------------------------------------*/
static void createExternalNetsForGlobal(
    dbMport globalMport,
    dbGlobal global,
    bool createTopLevelPorts)
{
    dbNetlist netlist = dbMportGetNetlist(globalMport);
    dbNetlist extNetlist;
    dbInst extInst;
    dbPort port;
    dbNet globalNet;
    utSym globalSym = dbGlobalGetSym(global);

    dbForeachNetlistExternalInst(netlist, extInst) {
        extNetlist = dbInstGetNetlist(extInst);
        port = dbPortCreate(extInst, globalMport);
        globalNet = dbNetlistFindNet(extNetlist, globalSym);
        if(globalNet == dbNetNull) {
            globalNet = dbGlobalNetCreate(extNetlist, global);
            threadGlobalNet(globalNet, createTopLevelPorts);
        }
        dbNetAppendPort(globalNet, port);
    } dbEndNetlistExternalInst;
}

/*--------------------------------------------------------------------------------------------------
  Thread the global net from here all the way up the hierarchy to the first netlist that contains
  a net on this net's global.
--------------------------------------------------------------------------------------------------*/
static void threadGlobalNet(
    dbNet net,
    bool createTopLevelPorts)
{
    dbNetlist netlist = dbNetGetNetlist(net);
    dbNetlist rootNetlist = dbDesignGetRootNetlist(dbNetlistGetDesign(netlist));
    dbGlobal global = dbNetGetGlobal(net);
    dbMport globalMport = dbMportNull;
    dbPort port;

    dbForeachNetPort(net, port) {
        if(dbInstGetType(dbPortGetInst(port)) == DB_FLAG) {
            globalMport = dbPortGetMport(port);
            convertExternalNetsToGlobal(globalMport, global, createTopLevelPorts);
        }
    } dbEndNetPort;
    if(globalMport == dbMportNull &&
            (createTopLevelPorts || (netlist != rootNetlist && dbNetlistVisited(netlist)))) {
        globalMport = dbMportCreate(netlist, dbGlobalGetSym(global), DB_PWR);
        dbFlagInstCreate(globalMport);
        dbNetAppendPort(net, dbMportGetFlagPort(globalMport));
        createExternalNetsForGlobal(globalMport, global, createTopLevelPorts);
    }
}

/*--------------------------------------------------------------------------------------------------
  Thread the global signal from the root netlist all the way down to where they are used.  Try to
  use mport names that match the global names.
--------------------------------------------------------------------------------------------------*/
void dbThreadGlobalThroughHierarchy(
    dbGlobal global,
    bool createTopLevelPorts)
{
    dbNet net;

    if(!createTopLevelPorts) {
        dbDesignMarkUsedNetlists(dbGlobalGetDesign(global));
    }
    dbForeachGlobalNet(global, net) {
        if(createTopLevelPorts || dbNetlistVisited(dbNetGetNetlist(net))) {
            threadGlobalNet(net, createTopLevelPorts);
        }
    } dbEndGlobalNet;
    if(createTopLevelPorts) {
        dbGlobalDestroy(global);
    }
}

/*--------------------------------------------------------------------------------------------------
  Thread global signals from the root netlist all the way down to where they are used.  Try to use
  mport names that match the global names.
--------------------------------------------------------------------------------------------------*/
void dbThreadGlobalsThroughHierarchy(
    dbDesign design,
    bool createTopLevelPorts)
{
    dbGlobal global;

    utDo {
        global = dbDesignGetFirstGlobal(design);
    } utWhile(global != dbGlobalNull) {
        dbThreadGlobalThroughHierarchy(global, createTopLevelPorts);
    } utRepeat;
}

/*--------------------------------------------------------------------------------------------------
  Merge two nets.  Delete the source net.
--------------------------------------------------------------------------------------------------*/
void dbMergeNetIntoNet(
    dbNet sourceNet,
    dbNet destNet)
{
    dbPort port;

    utDo {
        port = dbNetGetFirstPort(sourceNet);
    } utWhile(port != dbPortNull) {
        dbNetRemovePort(sourceNet, port);
        dbNetAppendPort(destNet, port);
    } utRepeat;
    dbNetDestroy(sourceNet);
}
