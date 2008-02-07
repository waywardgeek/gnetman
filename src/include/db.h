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

/* A quick note about bus ordering:

   Nets on busses, and mports on mbusses are ordered from left index to right index, going from 0
   to the number of elements.  This is also the order of mports on a netlist, and thus the order
   of ports on instances.
*/

#include "dbdatabase.h"

/* Top level functions */
void dbStart(void);
void dbStop(void);
void dbShortStart(void);
void dbShortStop(void);

/* Constructors */
dbDesign dbDesignCreate(utSym name, dbDesign libraryDesign);
dbNetlist dbNetlistCreate(dbDesign design, utSym name, dbNetlistType type, utSym device);
dbMport dbMportCreate(dbNetlist netlist, utSym name, dbMportType type);
dbMbus dbMbusCreate(dbNetlist netlist, utSym name, dbMportType type, uint32 left, uint32 right);
dbPort dbPortCreate(dbInst inst, dbMport mport);
dbInst dbInstCreate(dbNetlist netlist, utSym name, dbNetlist internalNetlist);
dbInst dbJoinInstCreate(dbNetlist netlist);
void dbJoinInstAddNet(dbInst join, dbNet net);
void dbJoinNets(dbNet net1, dbNet net2);
dbInst dbFlagInstCreate(dbMport mport);
dbInst dbBusFlagInstCreate(dbMbus mbus);
dbNet dbNetCreate(dbNetlist netlist, utSym name);
dbNet dbGlobalNetCreate(dbNetlist netlist, dbGlobal global);
dbNet dbNetlistGetOneNet(dbNetlist netlist);
dbNet dbNetlistGetZeroNet(dbNetlist netlist);
dbNet dbNetFindOrCreate(dbNetlist netlist, utSym name);
dbBus dbBusCreate(dbNetlist netlist, utSym name, uint32 minIndex, uint32 maxIndex);
dbMportType dbInvertMportType(dbMportType type);
dbGlobal dbGlobalCreate(dbDesign design, utSym name);
dbAttr dbAttrCreate(utSym  name, utSym value);
dbAttr dbAttrCopy(dbAttr attr);
dbAttr dbCopyAttrs(dbAttr attr);
dbDevspec dbDevspecCreate(utSym name, dbSpiceTargetType type, char *deviceString);

/* Shortcuts */
char *dbInstGetUserName(dbInst inst);
void dbDesignVisitNetlists(dbDesign design, void (*func)(dbNetlist));
void dbNetlistClearMportVisitedFlags(dbNetlist netlist);
void dbDesignClearNetlistVisitedFlags(dbDesign design);
void dbNetlistClearNetVisitedFlags(dbNetlist netlist);
void dbDesignMarkUsedNetlists(dbDesign design);
char *dbPortGetName(dbPort port);
utSym dbDesignCreateUniqueNetlistName(dbDesign design, char *name);
utSym dbDesignCreateUniqueGlobalName(dbDesign design, char *name);
utSym dbNetlistCreateUniqueNetName(dbNetlist netlist, char *name);
utSym dbNetlistCreateUniqueInstName(dbNetlist netlist, char *name);
utSym dbNetlistCreateUniqueMportName(dbNetlist netlist, char *name);
utSym dbFindPinTypeSym(dbMportType type);
dbMportType dbFindPinTypeFromSym(utSym pinType);
utSym dbFindDirectionSym(dbDirection direction);
dbDirection dbFindDirectionFromSym(utSym directionSym);
dbDirection dbFindMportTypeDirection(dbMportType type);
bool dbNetIsZeroOrOne(dbNet net);
bool dbNetIsOne(dbNet net);
bool dbNetIsZero(dbNet net);
void dbInstSetValue(dbInst inst, utSym name, utSym value);
utSym dbInstGetValue(dbInst inst, utSym name);
void dbNetSetValue(dbNet net, utSym name, utSym value);
utSym dbNetGetValue(dbNet net, utSym name);
void dbNetlistSetValue(dbNetlist netlist, utSym name, utSym value);
utSym dbNetlistGetValue(dbNetlist netlist, utSym name);
void dbDesignSetValue(dbDesign design, utSym name, utSym value);
utSym dbDesignGetValue(dbDesign design, utSym name);
void dbMportSetValue(dbMport mport, utSym name, utSym value);
utSym dbMportGetValue(dbMport mport, utSym name);
#define dbInstGetType(inst) dbNetlistGetType(dbInstGetInternalNetlist(inst))
double dbFindScalingFactor(char *string);
char *dbFindNetlistTypeName(dbNetlistType type);
bool dbInstIsGraphical(dbInst inst);
bool dbNameHasRange(char *name, uint32 *left, uint32 *right);

/* Queries */
dbPort dbFindPortFromInstMport(dbInst inst, dbMport mport);
utSym dbFindAttrValue(dbAttr attr, utSym name);
dbAttr dbFindAttr(dbAttr attr, utSym name);
dbAttr dbFindAttrNoCase(dbAttr attr, utSym name);
dbSpiceTargetType dbFindSpiceTargetFromName(char *name);
char *dbGetSpiceTargetName(dbSpiceTargetType type);
dbDevspec dbFindCurrentDevspec(void);
dbGlobal dbFindNetFromNetlistGlobal(dbNetlist netlist, dbGlobal global);
dbNetlistFormat dbFindNetlistFormat(char *format);

/* Netlist and design manipulations */
void dbDesignSetNetNamesToMatchMports(dbDesign design);
void dbDesignBuildNetsForFloatingPorts(dbDesign design);
void dbNetlistSetNetNamesToMatchMports(dbNetlist netlist);
void dbDesignMakeNetlistNamesUpperCase(dbDesign design);
void dbInstRename(dbInst inst, utSym newName);
void dbNetRename(dbNet net, utSym newName);
void dbGlobalRename(dbGlobal global, utSym newName);
void dbNetlistRename(dbNetlist netlist, utSym newName);
void dbDesignConvertPowerInstsToGlobals(dbDesign design);
void dbDesignExplodeArrayInsts(dbDesign design);
void dbNetlistExplodeArrayInsts(dbNetlist netlist);
void dbInstExplode(dbInst inst);
void dbDesignEliminateNonAlnumChars(dbDesign design);
void dbNetlistEliminateNonAlnumChars(dbNetlist netlist);
void dbInstReplaceInternalNetlist(dbInst inst, dbNetlist newInternalNetlist);
void dbBusHookup(dbBus bus, dbPort firstPort);
extern void dbThreadGlobalsThroughHierarchy(dbDesign design, bool createTopLevelPorts);
extern void dbThreadGlobalThroughHierarchy(dbGlobal global, bool createTopLevelPorts);
void dbMergeNetIntoNet(dbNet sourceNet, dbNet destNet);
dbMport dbMbusIndexMport(dbMbus mbus, uint32 bit);
dbNet dbBusIndexNet(dbBus bus, uint32 bit);

extern dbRoot dbTheRoot;

/*#define dbCurrentDesign dbRootGetCurrentDesign(dbTheRoot)*/
#define dbCurrentLibrary dbRootGetCurrentLibrary(dbTheRoot)
#define dbCurrentNetlist dbRootGetCurrentNetlist(dbTheRoot)
#define dbGschemComponentPath dbRootGetGschemComponentPath(dbTheRoot)
#define dbGschemSourcePath dbRootGetGschemSourcePath(dbTheRoot)
#define dbSpiceTarget dbRootGetSpiceTarget(dbTheRoot)
#define dbDefaultOneSym dbRootGetDefaultOneSym(dbTheRoot)
#define dbDefaultZeroSym dbRootGetDefaultZeroSym(dbTheRoot)
#define dbMaxLineLength dbRootGetMaxLineLength(dbTheRoot)
#define dbIncludeTopLevelPorts dbRootIncludeTopLevelPorts(dbTheRoot)
#define dbLibraryWins dbRootLibraryWins(dbTheRoot)

/* Temp hack to set reistor names */
extern utSym geRES250Sym, geRES6KSym;
extern utSym dbGraphicalSym;
