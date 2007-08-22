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
  Write out a SPICE design.

  A limitation in the .cir format seems to be the inability to have ports with different names
  than the signals they represent.  This makes it impossible to model feed-throughs.
--------------------------------------------------------------------------------------------------*/
#include <stdio.h>
#include <string.h>
#include "cir.h"

FILE *cirFile;
uint32 cirLineNum;
cirRoot cirTheRoot;

static char *cirLine;
static uint32 cirLinePos, cirLineSize, cirLastBreakPos;
static bool cirInComment;
static uint32 cirMaxLineLength;

utSym cirGraphicalSym, cirNetSym, cirSpiceTypeSym, cirSpiceTextSym;

char *cirDeviceStringPtr;
dbSpiceTargetType cirTarget;

/*--------------------------------------------------------------------------------------------------
  These global variables are set based on the target type, and control various aspects of the
  generated netlists.
--------------------------------------------------------------------------------------------------*/
bool cirUseBracesAroundParams, cirDeclareParameters;
char cirHierarchySeparator;

/*--------------------------------------------------------------------------------------------------
  Get the name of the net, or it's global if it exists.
--------------------------------------------------------------------------------------------------*/
static char *getNetName(
    dbNet net)
{
    dbGlobal global = dbNetGetGlobal(net);

    if(global != dbGlobalNull) {
        return dbGlobalGetName(global);
    }
    return dbNetGetName(net);
}

/*--------------------------------------------------------------------------------------------------
  Build devices from the definition file or string.
--------------------------------------------------------------------------------------------------*/
bool cirBuildDevices(void)
{
    dbDevspec devspec = dbFindCurrentDevspec();
    cirDeviceStringPtr = dbDevspecGetString(devspec);

    if(utSetjmp()) {
        return false;
    }
    cirLineNum = 1;
    if(cirparse()) {
        utUnsetjmp();
        return false;
    }
    utUnsetjmp();
    return true;
}

/*--------------------------------------------------------------------------------------------------
  Build param objects for each {IDENT} parameter used in the attribute value.
--------------------------------------------------------------------------------------------------*/
cirParam cirParamCreate(
    dbNetlist netlist,
    utSym sym)
{
    cirParam param = cirNetlistFindParam(netlist, sym);

    if(param != cirParamNull) {
        return param;
    }
    param = cirParamAlloc();
    cirParamSetSym(param, sym);
    cirNetlistInsertParam(netlist, param);
    return param;
}

/*--------------------------------------------------------------------------------------------------
  Build param objects for each {IDENT} parameter used in the attribute value.
--------------------------------------------------------------------------------------------------*/
static void buildParamsUsedInAttr(
    dbNetlist netlist,
    dbAttr attr)
{
    utSym value = dbAttrGetValue(attr);
    char *buf, *p;

    if(value == utSymNull) {
        return;
    }
    buf = utCopyString(utSymGetName(value));
    do {
        p = strchr(buf, '{');
        if(p != NULL) {
            *p++ = '\0';
            buf = p;
            p = strchr(buf, '}');
            if(p != NULL) {
                *p++ = '\0';
                cirParamCreate(netlist, utSymCreate(buf));
                buf = p;
            }
        }
    } while(p != NULL);
}

/*--------------------------------------------------------------------------------------------------
  Find all attributes on the instance of the form {IDENT}.  Build param objects for them.
--------------------------------------------------------------------------------------------------*/
static void buildInstParams(
    dbInst inst)
{
    dbNetlist netlist = dbInstGetNetlist(inst);
    dbNetlist internalNetlist = dbInstGetInternalNetlist(inst);
    dbAttr attr;

    for(attr = dbInstGetAttr(inst); attr != dbAttrNull; attr = dbAttrGetNextAttr(attr)) {
        buildParamsUsedInAttr(netlist, attr);
    }
    for(attr = dbNetlistGetAttr(internalNetlist); attr != dbAttrNull;
            attr = dbAttrGetNextAttr(attr)) {
        buildParamsUsedInAttr(netlist, attr);
    }
}

/*--------------------------------------------------------------------------------------------------
  Find all attributes on instances in the netlist of the form {IDENT}.  These can be set when
  the netlist is instantiated as a sub-circuit.
--------------------------------------------------------------------------------------------------*/
static void buildNetlistParams(
    dbNetlist netlist)
{
    dbInst inst;
    dbAttr attr;

    for(attr = dbNetlistGetAttr(netlist); attr != dbAttrNull; attr = dbAttrGetNextAttr(attr)) {
        if(dbAttrDeclared(attr)) {
            cirParamCreate(netlist, dbAttrGetName(attr));
        }
    }
    dbForeachNetlistInst(netlist, inst) {
        buildInstParams(inst);
    } dbEndNetlistInst;
}

/*--------------------------------------------------------------------------------------------------
  Extend database netlists so we can find out what parameters they take.
--------------------------------------------------------------------------------------------------*/
static void extendNetlists(
    dbDesign design)
{
    dbDesign libDesign = dbDesignGetLibraryDesign(design);
    dbNetlist netlist;

    dbForeachDesignNetlist(design, netlist) {
        buildNetlistParams(netlist);
    } dbEndDesignNetlist;
    if(libDesign != dbDesignNull) {
        extendNetlists(libDesign);
    }
}

/*--------------------------------------------------------------------------------------------------
  Print any remaining characters that are in the buffer.
--------------------------------------------------------------------------------------------------*/
static void flushBuffer(void)
{
    if(cirLinePos > 0) {
        if(cirLinePos == cirLineSize) {
            cirLineSize <<= 1;
            utResizeArray(cirLine, cirLineSize);
        }
        cirLine[cirLinePos] = '\0';
        fprintf(cirFile, "%s", cirLine);
        cirLinePos = 0;
    }
}

/*--------------------------------------------------------------------------------------------------
  Add a character to the current line buffer.  If we reach a newline, print the line, or break
  the line if it is too long.
--------------------------------------------------------------------------------------------------*/
static void addChar(
    char c)
{
    uint32 linePos;

    if(cirLinePos + 2 >= cirLineSize) {
        cirLineSize <<= 1;
        utResizeArray(cirLine, cirLineSize);
    }
    if(cirLinePos == 0) {
        if(c == '*') {
            cirInComment = true;
        } else {
            cirInComment = false;
        }
    }
    cirLine[cirLinePos++] = c;
    if(c == '\n') {
        cirLine[cirLinePos] = '\0';
        fprintf(cirFile, "%s", cirLine);
        cirLinePos = 0;
    } else {
        if(c == ' ') {
            cirLastBreakPos = cirLinePos - 1;
        }
        if(cirLastBreakPos != UINT32_MAX && cirLinePos >= cirMaxLineLength) {
            cirLine[cirLastBreakPos++] = '\0';
            fprintf(cirFile, "%s\n", cirLine);
            linePos = 0;
            if(cirInComment) {
                cirLine[linePos++] = '*';
            } else {
                cirLine[linePos++] = '+';
            }
            while(cirLastBreakPos < cirLinePos) {
                cirLine[linePos++] = cirLine[cirLastBreakPos++];
            }
            cirLinePos = linePos;
        }
    }
}

/*--------------------------------------------------------------------------------------------------
  Write to cirFile.
--------------------------------------------------------------------------------------------------*/
void cirPrint(
    char *format,
    ...)
{
    char *line;
    va_list ap;

    va_start(ap, format);
    line = utVsprintf(format, ap);
    va_end(ap);
    while(*line != '\0') {
        addChar(*line);
        line++;
    }
}

/*--------------------------------------------------------------------------------------------------
  Write global net definitions out.
--------------------------------------------------------------------------------------------------*/
static void writeGlobals(
    dbDesign design)
{
    dbGlobal global;

    dbForeachDesignGlobal(design, global) {
        cirPrint(".global %s\n", dbGlobalGetName(global));
    } dbEndDesignGlobal;
    cirPrint("\n");
}

/*--------------------------------------------------------------------------------------------------
  Write out the netlist's mports.
--------------------------------------------------------------------------------------------------*/
static void writeMports(
    dbNetlist netlist)
{
    dbMport mport;

    dbForeachNetlistMport(netlist, mport) {
        cirPrint(" %s", dbMportGetName(mport));
    } dbEndNetlistMport;
}

/*--------------------------------------------------------------------------------------------------
  If needed, strip out the braces from the string.
--------------------------------------------------------------------------------------------------*/
static char *preProcessValue(
    char *value)
{
    char *string, c, *prev, *next;

    if(cirUseBracesAroundParams) {
        return value;
    }
    string = utCopyString(value);
    prev = string;
    next = string;
    utDo {
        c = *next;
    } utWhile(c != '\0') {
        if(c != '{' && c != '}') {
            *prev++ = c;
        }
        next++;
    } utRepeat;
    *prev = '\0';
    return string;
}

/*--------------------------------------------------------------------------------------------------
  Write out parameters passed to the subcircuit.
--------------------------------------------------------------------------------------------------*/
static void writeSubcircuitParameters(
    dbInst inst)
{
    dbNetlist netlist = dbInstGetInternalNetlist(inst);
    cirParam param;
    utSym name, value;

    cirForeachNetlistParam(netlist, param) {
        name = cirParamGetSym(param);
        value = dbFindAttrValue(dbInstGetAttr(inst), name);
        if(value == utSymNull) {
            value = dbFindAttrValue(dbNetlistGetAttr(netlist), name);
        }
        if(value != utSymNull) {
            cirPrint(" %s=%s", utSymGetName(name), preProcessValue(utSymGetName(value)));
        }
    } cirEndNetlistParam;
}

/*--------------------------------------------------------------------------------------------------
  Write out the instance as a subcircuit instance.  Ports are always in the same order as mports.
--------------------------------------------------------------------------------------------------*/
static void writeSubcircuitInst(
    dbInst inst)
{
    dbNetlist internalNetlist = dbInstGetInternalNetlist(inst);
    dbNet net;
    dbPort port;

    cirPrint("X%s", dbInstGetUserName(inst));
    dbForeachInstPort(inst, port) {
        net = dbPortGetNet(port);
        cirPrint(" %s", getNetName(net));
    } dbEndInstPort;
    cirPrint(" %s", dbNetlistGetName(internalNetlist));
    writeSubcircuitParameters(inst);
    cirPrint("\n");
}

/*--------------------------------------------------------------------------------------------------
  Write the device attributes for this instance.
--------------------------------------------------------------------------------------------------*/
static void writeDeviceAttr(
    cirAttr attr,
    dbInst inst)
{
    dbNetlist internalNetlist = dbInstGetInternalNetlist(inst);
    dbAttr dbattr = dbFindAttrNoCase(dbInstGetAttr(inst), cirAttrGetSym(attr));
    utSym value;

    if(dbattr == dbAttrNull) {
        dbattr = dbFindAttrNoCase(dbNetlistGetAttr(internalNetlist), cirAttrGetSym(attr));
    }
    if(dbattr == dbAttrNull) {
        if(cirAttrOptional(attr)) {
            return;
        }
        utWarning("Instance %s in netlist %s is missing manditory attribute %s",
            dbInstGetUserName(inst), dbNetlistGetName(dbInstGetNetlist(inst)), cirAttrGetName(attr));
        cirPrint(" <missing_%s>", cirAttrGetName(attr));
    } else {
        value = dbAttrGetValue(dbattr);
        if(cirAttrNameVisible(attr)) {
            if(!cirAttrNameVisible(attr)) {
                cirPrint(" %s", cirAttrGetName(attr));
            } else {
                if(value == utSymNull) {
                    utWarning("Value expected for attribute %s on instance %s in netlist %s",
                        cirAttrGetName(attr), dbInstGetUserName(inst),
                        dbNetlistGetName(dbInstGetNetlist(inst)));
                } else {
                    cirPrint(" %s=%s", cirAttrGetName(attr), preProcessValue(utSymGetName(value)));
                }
            }
        } else {
            if(value == utSymNull) {
                utWarning("Value expected for attribute %s on instance %s in netlist %s",
                    cirAttrGetName(attr), dbInstGetUserName(inst),
                    dbNetlistGetName(dbInstGetNetlist(inst)));
            } else {
                cirPrint(" %s", preProcessValue(utSymGetName(value)));
            }
        }
    }
}

/*--------------------------------------------------------------------------------------------------
  Write the device attributes for this instance.
--------------------------------------------------------------------------------------------------*/
static void writeDeviceAttrs(
    cirDevice device,
    dbInst inst)
{
    cirAttr attr;

    cirForeachDeviceAttr(device, attr) {
        writeDeviceAttr(attr, inst);
    } cirEndDeviceAttr;
}

/*--------------------------------------------------------------------------------------------------
  If the pin name is listed in the value, return the net, otherwise NULL.
--------------------------------------------------------------------------------------------------*/
static char *findDefaultNetFromValue(
    utSym value,
    char *pinName)
{
    char *buf, *p, *netName, *portName;

    if(value == utSymNull) {
        return NULL;
    }
    buf = utCopyString(utSymGetName(value));
    p = strchr(buf, ':');
    if(p == NULL) {
        return NULL;
    }
    *p++ = '\0';
    netName = buf;
    do {
        portName = p;
        p = strchr(p, ',');
        if(p != NULL) {
            *p++ = '\0';
        }
        if(strcmp(portName, pinName) == 0) {
            return netName;
        }
    } while(p != NULL);
    return NULL;
}

/*--------------------------------------------------------------------------------------------------
  Write the device instance's pin connection.
--------------------------------------------------------------------------------------------------*/
static char *findDefaultNetForPin(
    dbInst inst,
    char *pinName)
{
    dbNetlist netlist;
    dbAttr attr;
    char *netName;

    for(attr = dbInstGetAttr(inst); attr != dbAttrNull; attr = dbAttrGetNextAttr(attr)) {
        if(dbAttrGetName(attr) == cirNetSym) {
            netName = findDefaultNetFromValue(dbAttrGetValue(attr), pinName);
            if(netName != NULL) {
                return netName;
            }
        }
    }
    netlist = dbInstGetInternalNetlist(inst);
    for(attr = dbNetlistGetAttr(netlist); attr != dbAttrNull; attr = dbAttrGetNextAttr(attr)) {
        if(dbAttrGetName(attr) == cirNetSym) {
            netName = findDefaultNetFromValue(dbAttrGetValue(attr), pinName);
            if(netName != NULL) {
                return netName;
            }
        }
    }
    return NULL;
}

/*--------------------------------------------------------------------------------------------------
  Write the device instance's pin connection.
--------------------------------------------------------------------------------------------------*/
static void writePin(
    cirPin pin,
    dbInst inst)
{
    dbNetlist internalNetlist = dbInstGetInternalNetlist(inst);
    dbNet net;
    dbMport mport = dbNetlistFindMport(internalNetlist, cirPinGetSym(pin));
    dbPort port = dbFindPortFromInstMport(inst, mport);
    char *pinName = cirPinGetName(pin);
    char *netName;

    if(port == dbPortNull) {
        netName = findDefaultNetForPin(inst, pinName);
        if(netName == NULL) {
            utWarning("Inst %s has no %s port... grounding", dbInstGetUserName(inst), pinName);
            cirPrint(" 0", pinName);
        } else {
            cirPrint(" %s", netName);
        }
    } else {
        net = dbPortGetNet(port);
        if(net == dbNetNull) {
            utWarning("Port %s in netlist %s has no net", dbPortGetName(port),
                dbNetlistGetName(dbInstGetNetlist(inst)));
            cirPrint(" <unconnected>");
        } else {
            cirPrint(" %s", getNetName(net));
        }
    }
}

/*--------------------------------------------------------------------------------------------------
  Write out the device instance.  Port ordering and attributes ordering are specified on the
  device object.
--------------------------------------------------------------------------------------------------*/
static bool writeDeviceInst(
    dbInst inst)
{
    dbNetlist internalNetlist = dbInstGetInternalNetlist(inst);
    cirDevice device;
    cirPin pin;
    dbAttr deviceAttrs = dbNetlistGetAttr(internalNetlist);
    utSym deviceType = dbFindAttrValue(deviceAttrs, cirSpiceTypeSym);
    char *instName = dbInstGetUserName(inst);
 
    if(dbFindAttrNoCase(deviceAttrs, cirGraphicalSym) != dbAttrNull) {
        return true; /* This is just a graphical thing */
    }
    if(deviceType == utSymNull) {
        /*
        utWarning("No spice type for symbol %s... skipping component %s",
            dbNetlistGetName(internalNetlist), dbInstGetUserName(inst));
        */
        return false; /* Write it as a subcircuit inst */
    }
    device = cirRootFindDevice(cirTheRoot, utSymGetLowerSym(deviceType));
    if(device == cirDeviceNull) {
        utWarning("Inst %s has no unknown device type %s", instName, utSymGetName(deviceType));
        return false;
    }
    cirPrint("%s", instName);
    cirForeachDevicePin(device, pin) {
        writePin(pin, inst);
    } dbEndInstPort;
    writeDeviceAttrs(device, inst);
    cirPrint("\n");
    return true;
}

/*--------------------------------------------------------------------------------------------------
  Write the instance out.
--------------------------------------------------------------------------------------------------*/
static void writeInst(
    dbInst inst)
{
    dbNetlist internalNetlist = dbInstGetInternalNetlist(inst);

    switch(dbNetlistGetType(internalNetlist)) {
    case DB_SUBCIRCUIT:
        writeSubcircuitInst(inst);
        break;
    case DB_DEVICE:
        if(!writeDeviceInst(inst)) {
            writeSubcircuitInst(inst);
        }
        break;
    case DB_POWER:
        utWarning("Power instance %s should have been converted to a global net",
            dbInstGetUserName(inst));
        break;
    case DB_FLAG:
        break;
    default:
        utExit("writeInst: Unknown netlist type");
    }
}

/*--------------------------------------------------------------------------------------------------
  Write out the netlist's instances.
--------------------------------------------------------------------------------------------------*/
static void writeInsts(
    dbNetlist netlist)
{
    dbInst inst;

    dbForeachNetlistInst(netlist, inst) {
        writeInst(inst);
    } dbEndNetlistInst;
}

/*--------------------------------------------------------------------------------------------------
  Write out any spicetext attributes.
--------------------------------------------------------------------------------------------------*/
static void writeSpiceText(
    dbNetlist netlist)
{
    dbAttr attr;
    bool printedText = false;

    for(attr = dbNetlistGetAttr(netlist); attr != dbAttrNull; attr = dbAttrGetNextAttr(attr)) {
        if(dbAttrGetName(attr) == cirSpiceTextSym) {
            cirPrint("%s\n", utSymGetName(dbAttrGetValue(attr)));
        }
    }
    if(printedText) {
        cirPrint("\n");
    }
}

/*--------------------------------------------------------------------------------------------------
  Write out the netlist's parameters, and default values.
--------------------------------------------------------------------------------------------------*/
static void writeParameterDeclarations(
    dbNetlist netlist)
{
    cirParam param;
    utSym name, value;

    cirForeachNetlistParam(netlist, param) {
        name = cirParamGetSym(param);
        value = dbFindAttrValue(dbNetlistGetAttr(netlist), name);
        if(value != utSymNull) {
            cirPrint(" %s=%s", utSymGetName(name), preProcessValue(utSymGetName(value)));
        } else {
            cirPrint(" %s=0", utSymGetName(name));
        }
    } cirEndNetlistParam;
}

static bool cirIncludeTopLevelPorts;
/*--------------------------------------------------------------------------------------------------
  Write out the netlist as a SPICE sub-circuit.
--------------------------------------------------------------------------------------------------*/
static void writeNetlist(
    dbNetlist netlist)
{
    dbDesign design;
    bool isRoot;

    if(dbNetlistGetType(netlist) != DB_SUBCIRCUIT) {
        return;
    }
    design = dbNetlistGetDesign(netlist);
    isRoot = dbDesignGetRootNetlist(design) == netlist;
    if(!isRoot || cirIncludeTopLevelPorts) {
        cirPrint(".subckt %s", dbNetlistGetName(netlist));
        writeMports(netlist);
        if(cirDeclareParameters) {
            writeParameterDeclarations(netlist);
        }
        cirPrint("\n");
    }
    writeInsts(netlist);
    writeSpiceText(netlist);
    if(!isRoot || cirIncludeTopLevelPorts) {
        cirPrint(".ends\n\n");
    }
}

/*--------------------------------------------------------------------------------------------------
  Write the child netlists of netlist so that they get declared first.
--------------------------------------------------------------------------------------------------*/
static void writeNetlistAndChildren(
    dbNetlist netlist)
{
    dbNetlist internalNetlist;
    dbInst inst;

    dbNetlistSetVisited(netlist, true);
    dbForeachNetlistInst(netlist, inst) {
        internalNetlist = dbInstGetInternalNetlist(inst);
        if(!dbNetlistVisited(internalNetlist)) {
            writeNetlistAndChildren(internalNetlist);
        }
    } dbEndNetlistInst;
    writeNetlist(netlist);
}

/*--------------------------------------------------------------------------------------------------
  Write the netlists of a design.  Black box netlists are not written.
--------------------------------------------------------------------------------------------------*/
static void writeNetlists(
    dbDesign design,
    bool wholeLibrary)
{
    dbNetlist netlist;

    if(wholeLibrary) {
        dbDesignClearNetlistVisitedFlags(design);
        dbForeachDesignNetlist(design, netlist) {
            if(!dbNetlistVisited(netlist)) {
                writeNetlistAndChildren(netlist);
            }
        } dbEndDesignNetlist;
    } else {
        dbDesignVisitNetlists(design, writeNetlist);
    }
}

/*--------------------------------------------------------------------------------------------------
  Write out a spice design.  If includeTopLevelPorts is set, the design is written out as a .subckt
--------------------------------------------------------------------------------------------------*/
static void writeDesign(
    dbDesign design,
    bool includeTopLevelPorts,
    bool wholeLibrary)
{
    cirIncludeTopLevelPorts = includeTopLevelPorts;
    writeGlobals(design);
    writeNetlists(design, wholeLibrary);
    if(!includeTopLevelPorts) {
        cirPrint(".end\n");
    }
}

/*--------------------------------------------------------------------------------------------------
  Initialize memory used in the spice netlister.
--------------------------------------------------------------------------------------------------*/
void cirStart(
    dbDesign design,
    dbSpiceTargetType targetType)
{
    cirDatabaseStart();
    if(design != dbDesignNull) {
        extendNetlists(design);
    }
    cirGraphicalSym = utSymCreate("graphical");
    cirNetSym = utSymCreate("net");
    cirSpiceTypeSym = utSymCreate("spicetype");
    cirSpiceTextSym = utSymCreate("spicetext");
    cirTheRoot = cirRootAlloc();
    cirLineSize = 42;
    cirLine = utNewA(char, cirLineSize);
    cirLinePos = 0;
    cirLastBreakPos = UINT32_MAX;
    cirTarget = targetType;
    cirUseBracesAroundParams = true;
    cirDeclareParameters = false;
    /* Set common parameters */
    switch(targetType) {
    case DB_PSPICE:
    case DB_LTSPICE:
        cirHierarchySeparator = ':';
        break;
    case DB_HSPICE:
    case DB_TCLSPICE:
    case DB_CDL:
        cirHierarchySeparator = '.';
        cirUseBracesAroundParams = false;
        cirDeclareParameters = true;
        break;
    default:
        utExit("Unknown spice target type");
    }
    /* Set target specific parameters */
    /* Note: there aren't any yet */
}

/*--------------------------------------------------------------------------------------------------
  Free memory used in the spice netlister.
--------------------------------------------------------------------------------------------------*/
void cirStop(void)
{
    utFree(cirLine);
    cirRootDestroy(cirTheRoot);
    cirDatabaseStop();
}

/*--------------------------------------------------------------------------------------------------
  Write out a SPICE design.  Note: This may modify the netlist if there are non-spice compatible
  things in it.  Consider making a copy of the netlist first if this is a problem.
--------------------------------------------------------------------------------------------------*/
bool cirWriteDesign(
    dbDesign design,
    char *fileName,
    bool includeTopLevelPorts,
    uint32 maxLineLength,
    bool wholeLibrary)
{
    char *exeName;
    char *localFileName = utNewA(char, strlen(fileName) + 1);

    utLogMessage("Writing SPICE file %s", fileName);
    if(maxLineLength > 0) {
        cirMaxLineLength = maxLineLength;
    } else {
        cirMaxLineLength = UINT32_MAX;
    }
    strcpy(localFileName, fileName);
    cirStart(design, dbSpiceTarget);
    if(!cirBuildDevices()) {
        utWarning("Could not build SPICE device configuration data");
    } else {
        cirFile = fopen(localFileName, "w");
        if(cirFile == NULL) {
            utWarning("Could not open file %s", localFileName);
            cirStop();
            utFree(localFileName);
            return false;
        }
        exeName = utBaseName(utGetExeFullPath());
        cirPrint("* Netlist %s created %s by %s version %s\n", localFileName,
            utGetDateAndTime(), exeName, utGetVersion());
        cirPrint("* Symbol search path = %s\n", dbGschemComponentPath);
        cirPrint("* Schematic search path = %s\n", dbGschemSourcePath);
        dbDesignSetNetNamesToMatchMports(design);
        dbDesignConvertPowerInstsToGlobals(design);
        dbDesignExplodeArrayInsts(design);
        dbDesignEliminateNonAlnumChars(design);
        writeDesign(design, includeTopLevelPorts, wholeLibrary);
        flushBuffer();
        fclose(cirFile);
    }
    cirStop();
    utFree(localFileName);
    return true;
}
