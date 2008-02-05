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
  Read in a SPICE design.
--------------------------------------------------------------------------------------------------*/
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "cir.h"

static dbDesign cirCurrentDesign, cirCurrentLibrary;
static dbNetlist cirCurrentNetlist, cirLastNetlist;
static char *cirLine;
static uint32 cirLinePosition, cirLineLength, cirMaxLineLength;
static char *cirFileName;
static bool cirSkipSubcircuit;
static bool cirCheckParameters;

/* Keywords */
static utSym cirSubcktSym, cirEndsSym, cirEndSym, cirGlobalSym, cirConnectSym;

#define cirGetc() (cirLine[cirLinePosition++])
#define cirUngetc() (cirLinePosition--)
#define cirPeekChar() (cirLine[cirLinePosition])

/*--------------------------------------------------------------------------------------------------
  Initialize keyworks.
--------------------------------------------------------------------------------------------------*/
static void initKeywords(void)
{
    cirSubcktSym = utSymCreate("subckt");
    cirEndsSym = utSymCreate("ends");
    cirEndSym = utSymCreate("end");
    cirGlobalSym = utSymCreate("global");
    cirConnectSym = utSymCreate("connect");
}

/*--------------------------------------------------------------------------------------------------
  We build an internal netlist for each device type up-front.  Later on, we delete the unused
  ones.
--------------------------------------------------------------------------------------------------*/
static void buildDeviceNetlist(
    cirDevice device)
{
    utSym name = dbDesignCreateUniqueNetlistName(cirCurrentDesign,
        utSprintf("spicedevice_%s", cirDeviceGetName(device)));
    dbNetlist netlist = dbNetlistCreate(cirCurrentDesign, name, DB_DEVICE, cirDeviceGetSym(device));
    cirPin pin;
    dbMport mport;
    dbAttr attr;

    cirDeviceSetInternalNetlist(device, netlist);
    cirForeachDevicePin(device, pin) {
        mport = dbMportCreate(netlist, cirPinGetSym(pin), DB_PAS);
    } cirEndDevicePin;
    attr = dbAttrCreate(cirSpiceTypeSym, cirDeviceGetSym(device));
    dbNetlistSetAttr(netlist, attr);
}

/*--------------------------------------------------------------------------------------------------
  We build an internal netlist for each device type up-front.  Later on, we delete the unused
  ones.
--------------------------------------------------------------------------------------------------*/
static void buildDeviceNetlists(void)
{
    cirDevice device;

    cirForeachRootDevice(cirTheRoot, device) {
        buildDeviceNetlist(device);
    } cirEndRootDevice;
}

/*--------------------------------------------------------------------------------------------------
  Free extended device netlist extensions, and also the unused database device netlists.
--------------------------------------------------------------------------------------------------*/
static void freeUnusedDeviceNetlists(void)
{
    cirDevice device;
    dbNetlist netlist;

    cirForeachRootDevice(cirTheRoot, device) {
        netlist = cirDeviceGetInternalNetlist(device);
        dbNetlistDestroy(netlist);
    } cirEndRootDevice;
}

/*--------------------------------------------------------------------------------------------------
  Just skip the current line.
--------------------------------------------------------------------------------------------------*/
static void skipLine(void)
{
    int c;

    do {
        c = getc(cirFile);
        if(c == '\n') {
            cirLineNum++;
            return;
        }
    } while(c != EOF);
}

/*--------------------------------------------------------------------------------------------------
  Add a character to the current line.
--------------------------------------------------------------------------------------------------*/
static void addChar(
    char c)
{
    if(cirLineLength == cirMaxLineLength) {
        cirMaxLineLength <<= 1;
        utResizeArray(cirLine, cirMaxLineLength);
    }
    cirLine[cirLineLength++] = c;
}

/*--------------------------------------------------------------------------------------------------
  Read a line into the local buffer.  Return false if we're at EOF.
--------------------------------------------------------------------------------------------------*/
static bool readLinePart(void)
{
    int c;

    do {
        c = getc(cirFile);
        if(c == '\n') {
            cirLineNum++;
        }
    } while(c != EOF && c <= ' ');
    if(c == EOF) {
        return false;
    }
    while(c != EOF && c != '\n') {
        addChar(c);
        c = getc(cirFile);
    }
    if(c == '\n') {
        cirLineNum++;
    }
    addChar('\0');
    return true;
}

/*--------------------------------------------------------------------------------------------------
  Determine if the next line starts with a +.
--------------------------------------------------------------------------------------------------*/
static bool lineStartsWithPlus(void)
{
    int c;

    do {
        c = getc(cirFile);
        if(c == '\n') {
            cirLineNum++;
        }
    } while(c != EOF && c < ' ');
    if(c == '+') {
        return true;
    }
    ungetc(c, cirFile);
    return false;
}

/*--------------------------------------------------------------------------------------------------
  Replace the first '$' with '\0'.
--------------------------------------------------------------------------------------------------*/
static void chopOffComment(void)
{
    char *p = strchr(cirLine, '$');

    if(p != NULL) {
        *p = '\0';
    }
}

/*--------------------------------------------------------------------------------------------------
  Read a line into the local buffer.  Return false if we're at EOF.  Read ahead to see if the
  next line starts with a +, and if it does, read that, too.
--------------------------------------------------------------------------------------------------*/
static bool readLine(void)
{
    cirLineLength = 0;
    cirLinePosition = 0;
    if(!readLinePart()) {
        return false;
    }
    while(lineStartsWithPlus()) {
        cirLine[cirLineLength - 1] = ' ';
        readLinePart();
    }
    if(dbSpiceTarget != DB_HSPICE) {
        chopOffComment();
    }
    return true;
}

/*--------------------------------------------------------------------------------------------------
  Just skip over space in the input line.  Return the first non-space character.
--------------------------------------------------------------------------------------------------*/
static char skipSpace(void)
{
    char c;

    utDo {
        c = cirPeekChar();
    } utWhile(c <= ' ' && c != '\0') {
        cirLinePosition++;
    } utRepeat;
    return cirPeekChar();
}

/*--------------------------------------------------------------------------------------------------
  Just determine if the character is part of an identifier.
--------------------------------------------------------------------------------------------------*/
static bool isIdentChar(
    char c)
{
    if(isalnum(c) || c == '_' || c == '$' || c == '#') {
        return true;
    }
    if(dbSpiceTarget == DB_HSPICE && (c == '[' || c == ']')) {
        return true;
    }
    if(dbSpiceTarget == DB_CDL && c == '!') {
        return true;
    }
    return false;
}

/*--------------------------------------------------------------------------------------------------
  Just read a SPICE identifier.
--------------------------------------------------------------------------------------------------*/
static utSym readIdentifier(void)
{
    utSym sym;
    char c = skipSpace();
    uint32 startPosition = cirLinePosition;

    if(isdigit(c) || c == '$' || !isIdentChar(c)) {
        return utSymNull;
    }
    while(isIdentChar(cirPeekChar())) {
        cirLinePosition++;
    }
    c = cirPeekChar();
    cirLine[cirLinePosition] = '\0';
    sym = utSymCreate(cirLine + startPosition);
    cirLine[cirLinePosition] = c;
    return sym;
}

/*--------------------------------------------------------------------------------------------------
  Just read a SPICE identifier, or integer, which is legal for node names.
--------------------------------------------------------------------------------------------------*/
static utSym readNodeName(void)
{
    utSym sym;
    char c = skipSpace();
    uint32 startPosition = cirLinePosition;

    if(c == '$' || !isIdentChar(c)) {
        return utSymNull;
    }
    while(isIdentChar(cirPeekChar())) {
        cirLinePosition++;
    }
    c = cirPeekChar();
    cirLine[cirLinePosition] = '\0';
    sym = utSymCreate(cirLine + startPosition);
    cirLine[cirLinePosition] = c;
    return sym;
}

/*--------------------------------------------------------------------------------------------------
  Just report the error and return.
--------------------------------------------------------------------------------------------------*/
void cirError(
    char *format,
    ...)
{
    char *buff;
    va_list ap;

    va_start(ap, format);
    buff = utVsprintf(format, ap);
    va_end(ap);
    utWarning("Error in file %s on line %u: %s", cirFileName, cirLineNum, buff);
}

/*--------------------------------------------------------------------------------------------------
  Find the net in the current netlist by this name, if it exists.  If it doesn't, create it.
  Also check for globals of the same name, and add it to the new net if it exists.
--------------------------------------------------------------------------------------------------*/
static dbNet findOrCreateNet(
    utSym netName)
{
    dbNet net = dbNetlistFindNet(cirCurrentNetlist, netName);
    dbGlobal global;

    if(net != dbNetNull) {
        return net;
    }
    net = dbNetCreate(cirCurrentNetlist, netName);
    global = dbDesignFindGlobal(cirCurrentDesign,  netName);
    if(global != dbGlobalNull) {
        dbGlobalInsertNet(global, net);
    }
    return net;
}

/*--------------------------------------------------------------------------------------------------
  Add ports to the instance.  We keep reading identifiers until we see the end or a parameter with
  an '='.  The last identifier was the subcircuit name, so we skip it.
--------------------------------------------------------------------------------------------------*/
static bool addInstPorts(
    dbInst inst)
{
    dbNetlist internalNetlist = dbInstGetInternalNetlist(inst);
    dbMport mport = dbNetlistGetFirstMport(internalNetlist);
    dbNet net;
    dbPort port;
    utSym netName, lastNetName = utSymNull;
    uint32 lastIdentifierPosition;
    char c;

    utDo {
        lastIdentifierPosition = cirLinePosition;
        netName = readNodeName();
        c = skipSpace();
    } utWhile(netName != utSymNull && c != '=') {
        if(lastNetName != utSymNull) {
            if(mport == dbMportNull) {
                cirError("Too many ports specified on instance %s", dbInstGetUserName(inst));
                return false;
            }
            port = dbPortCreate(inst, mport);
            net = findOrCreateNet(lastNetName);
            dbNetInsertPort(net, port);
            mport = dbMportGetNextNetlistMport(mport);
        }
        lastNetName = netName;
    } utRepeat;
    if(c != '=' && c != '\0') {
        cirError("Unexpected character '%c'", c);
        return false;
    }
    if(c == '=') {
        /* Push back the identifier so it can be read as a parameter */
        cirLinePosition = lastIdentifierPosition;
    }
    if(mport != dbMportNull) {
        cirError("Not enough ports specified on instance %s", dbInstGetUserName(inst));
        return false;
    }
    return true;
}

/*--------------------------------------------------------------------------------------------------
  Check that the parameter is actually declared on the sub-circuit.
--------------------------------------------------------------------------------------------------*/
static void checkParameter(
    dbInst inst,
    utSym name)
{
    dbNetlist internalNetlist = dbInstGetInternalNetlist(inst);
    dbAttr attr = dbFindAttr(dbNetlistGetAttr(internalNetlist), name);

    if(attr == dbAttrNull) {
        cirError("Parameter %s not declared on sub-circuit %s... Turning off parameter checking",
            utSymGetName(name), dbNetlistGetName(internalNetlist));
        cirCheckParameters = false;
    }
}

/*--------------------------------------------------------------------------------------------------
  Read a value.  Return utSymNull if there are no characters in the value.
--------------------------------------------------------------------------------------------------*/
static utSym readValue(void)
{
    utSym value;
    uint32 startPos;
    char c;

    skipSpace();
    startPos = cirLinePosition;
    do {
        c = cirGetc();
    } while(c > ' ');
    cirLinePosition--;
    if(cirLinePosition == startPos) {
        return utSymNull;
    }
    cirLine[cirLinePosition] = '\0';
    value = utSymCreate(cirLine + startPos);
    cirLine[cirLinePosition] = c;
    return value;
}

/*--------------------------------------------------------------------------------------------------
  Read the parameter.
--------------------------------------------------------------------------------------------------*/
static dbAttr readParameter(void)
{
    utSym name = readIdentifier();
    utSym value;

    if(name == utSymNull) {
        return dbAttrNull;
    }
    if(cirGetc() != '=') {
        cirError("Expected '=' after attribute %s", utSymGetName(name));
        return dbAttrNull;
    }
    value = readValue();
    return dbAttrCreate(name, value);
}

/*--------------------------------------------------------------------------------------------------
  Read the parameter, and add it to the instance as an attribute.
--------------------------------------------------------------------------------------------------*/
static bool readInstParameter(
    dbInst inst)
{
    dbAttr attr = readParameter();

    if(attr == dbAttrNull) {
        return false;
    }
    if(cirCheckParameters) {
        checkParameter(inst, dbAttrGetName(attr));
    }
    dbAttrSetNextAttr(attr, dbInstGetAttr(inst));
    dbInstSetAttr(inst, attr);
    return true;
}

/*--------------------------------------------------------------------------------------------------
  Add parameters to the instance.  These are the remaining IDENT= things left on the line.
  Without parsing the actual expressions, it's tough to know where to end.  We look for the next
  IDENT= thing, and end just before it.
--------------------------------------------------------------------------------------------------*/
static bool addInstParameters(
    dbInst inst)
{
    char c;

    utDo {
        c = skipSpace();
    } utWhile(c != '\0') {
        if(!readInstParameter(inst)) {
            return false;
        }
    } utRepeat;
    return true;
}

/*--------------------------------------------------------------------------------------------------
  Skip the next identifier.  Return false if the next token is not an identifier.
--------------------------------------------------------------------------------------------------*/
static bool skipNodeName(void)
{
    char c = skipSpace();

    if(!isIdentChar(c)) {
        return false;
    }
    while(isIdentChar(cirPeekChar())) {
        cirLinePosition++;
    }
    return true;
}

/*--------------------------------------------------------------------------------------------------
  Find the last non-parameter identifier.  This is the name of the sub-circuit.
--------------------------------------------------------------------------------------------------*/
static utSym findSubCircuitSym(void)
{
    uint32 linePosition = cirLinePosition;
    uint32 lastPosition1 = linePosition;
    uint32 lastPosition2 = linePosition;
    uint32 lastPosition3 = linePosition;
    utSym sym;
    char c;

    while(skipNodeName()) {
        lastPosition3 = lastPosition2;
        lastPosition2 = lastPosition1;
        lastPosition1 = cirLinePosition;
    }
    c = skipSpace();
    if(c == '=') {
        cirLinePosition = lastPosition3;
    } else if(c == '\0') {
        cirLinePosition = lastPosition2;
    } else {
        cirLinePosition = linePosition;
        return utSymNull;
    }
    sym = readIdentifier();
    cirLinePosition = linePosition;
    return sym;
}

/*--------------------------------------------------------------------------------------------------
  Create an instance from the X statment.
  HSPICE example: XX3 N2 slave CKI CLKN passtrans P=.6u N=.3u
--------------------------------------------------------------------------------------------------*/
static bool executeInstance(void)
{
    dbNetlist internalNetlist = dbNetlistNull;
    dbNetlist libraryNetlist = dbNetlistNull;
    dbInst inst;
    utSym name = readIdentifier();
    utSym internalNetlistSym;

    if(name == utSymNull) {
        cirError("Bad identifier for instance name");
        return false;
    }
    internalNetlistSym = findSubCircuitSym();
    if(internalNetlistSym != utSymNull) {
        internalNetlist = dbDesignFindNetlist(cirCurrentDesign, internalNetlistSym);
        if(cirCurrentLibrary != dbDesignNull) {
            libraryNetlist = dbDesignFindNetlist(cirCurrentLibrary, internalNetlistSym);
            if(internalNetlist == dbNetlistNull || (dbLibraryWins && libraryNetlist !=
                    dbNetlistNull)) {
                internalNetlist = libraryNetlist;
            }
        }
        if(internalNetlist == dbNetlistNull) {
            /*cirError("Unable to find subcircuit '%s'", utSymGetName(internalNetlistSym));*/
            internalNetlist = dbNetlistCreate(cirCurrentDesign, internalNetlistSym, DB_UNDEFINED, utSymNull);
            return false;
        }
    } else {
        cirError("Unable to parse instance name");
        return false;
    }
    inst = dbInstCreate(cirCurrentNetlist, name, internalNetlist);
    if(!addInstPorts(inst)) {
        return false;
    }
    if(!addInstParameters(inst)) {
        return false;
    }
    return true;
}

/*--------------------------------------------------------------------------------------------------
  Build a new mport with the given name.
--------------------------------------------------------------------------------------------------*/
static dbMport buildMport(
    utSym name)
{
    dbMport mport = dbMportCreate(cirCurrentNetlist, name, DB_PAS);
    dbInst flag = dbFlagInstCreate(mport);
    dbNet net = findOrCreateNet(name);

    dbNetInsertPort(net, dbInstGetFirstPort(flag));
    return mport;
}

/*--------------------------------------------------------------------------------------------------
  Build mports on the current netlist.  Stop at the first parameter.
--------------------------------------------------------------------------------------------------*/
static bool buildMports(void)
{
    utSym netName, lastNetName = utSymNull;
    char c;
    uint32 lastIdentifierPosition;

    utDo {
        lastIdentifierPosition = cirLinePosition;
        netName = readNodeName();
        c = cirPeekChar();
    } utWhile(netName != utSymNull && c != '=') {
        if(lastNetName != utSymNull) {
            buildMport(lastNetName);
        }
        lastNetName = netName;
    } utRepeat;
    if(c == '\0' || c == '=') {
        if(lastNetName != utSymNull) {
            buildMport(lastNetName);
            if(c == '=') {
                /* Put the identifier back so it can be read as an attribute name */
                cirLinePosition = lastIdentifierPosition;
            }
        }
    } else {
        cirError("Unexpected character '%c'", c);
        return false;
    }
    return true;
}

/*--------------------------------------------------------------------------------------------------
  Read the parameter, and add it to the netlist as an attribute.
--------------------------------------------------------------------------------------------------*/
static bool readNetlistParameter(void)
{
    dbAttr attr = readParameter();

    if(attr == dbAttrNull) {
        return false;
    }
    dbAttrSetNextAttr(attr, dbNetlistGetAttr(cirCurrentNetlist));
    dbNetlistSetAttr(cirCurrentNetlist, attr);
    dbAttrSetDeclared(attr, true);
    return true;
}

/*--------------------------------------------------------------------------------------------------
  Add the netlist parameters.
--------------------------------------------------------------------------------------------------*/
static bool addNetlistParameters(void)
{
    while(skipSpace() != '\0') {
        if(!readNetlistParameter()) {
            return false;
        }
    }
    return true;
}

/*--------------------------------------------------------------------------------------------------
  Execute the directive.

  Expample: .subckt passtrans D1 D2 EN E P=2u N=1u
--------------------------------------------------------------------------------------------------*/
static bool executeSubckt(void)
{
    utSym netlistName = readIdentifier();

    if(netlistName == utSymNull) {
        cirError("Bad sub-circuit name");
    }
    cirCurrentNetlist = dbDesignFindNetlist(cirCurrentDesign, netlistName);
    if(cirCurrentNetlist != dbNetlistNull) {
        if(dbNetlistGetType(cirCurrentNetlist) == DB_SUBCIRCUIT) {
            cirError("Redefinition of sub-circuit %s -- using old circuit",
                utSymGetName(netlistName));
            /* Skip to next sub-circuit */
            cirSkipSubcircuit = true;
            return true;
        } 
        dbNetlistSetType(cirCurrentNetlist, DB_SUBCIRCUIT);
    } else {
        cirCurrentNetlist = dbNetlistCreate(cirCurrentDesign, netlistName, DB_SUBCIRCUIT, utSymNull);
    }

    if(!buildMports()) {
        return false;
    }
    return addNetlistParameters();
}

/*--------------------------------------------------------------------------------------------------
  Execute the global directive.
--------------------------------------------------------------------------------------------------*/
static bool executeGlobal(void)
{
    utSym name;

    utDo {
        name = readNodeName();
    } utWhile(name != utSymNull) {
        dbGlobalCreate(cirCurrentDesign, name);
    } utRepeat;
    return skipSpace() == '\0';
}

/*--------------------------------------------------------------------------------------------------
  Execute the connect directive.
--------------------------------------------------------------------------------------------------*/
static bool executeConnect(void)
{
    dbInst join;
    dbNet net;
    utSym netName;

    if(cirCurrentNetlist == dbNetlistNull) {
        cirError("Connect statements outside subckt blocks are ignored");
        return true;
    }
    join = dbJoinInstCreate(cirCurrentNetlist);
    utDo {
        netName = readNodeName();
    } utWhile(netName != utSymNull) {
        net = findOrCreateNet(netName);
        dbJoinInstAddNet(join, net);
    } utRepeat;
    return skipSpace() == '\0';
}

/*--------------------------------------------------------------------------------------------------
  Execute the directive.
--------------------------------------------------------------------------------------------------*/
static bool executeDirective(void)
{
    utSym directive = utSymGetLowerSym(readIdentifier());

    if(directive == cirEndsSym) {
        if(cirCurrentNetlist == dbNetlistNull) {
            cirError(".ends directive without .subckt");
        }
        if(cirSkipSubcircuit) {
            cirSkipSubcircuit = false;
        }
        cirLastNetlist = cirCurrentNetlist;
        cirCurrentNetlist = dbNetlistNull;
        return true;
    } else if(cirSkipSubcircuit) {
        return true;
    } else if(directive == cirSubcktSym) {
        return executeSubckt();
    } else if(directive == cirEndSym) {
        return true;
    } else if(directive == cirGlobalSym) {
        return executeGlobal();
    } else if(directive == cirConnectSym) {
        return executeConnect();
    } else {
        cirError("Directive %s is currently unsupported", utSymGetName(directive));
    }
    return false;
}

/*--------------------------------------------------------------------------------------------------
  Read a pin on the device.  Return false if we have parsing trouble.
--------------------------------------------------------------------------------------------------*/
static bool readPin(
    cirPin pin,
    dbInst inst)
{
    dbNetlist internalNetlist = dbInstGetInternalNetlist(inst);
    dbMport mport = dbNetlistFindMport(internalNetlist, cirPinGetSym(pin));
    dbNet net;
    dbPort port;
    utSym netName = readNodeName();

    if(netName == utSymNull) {
        cirError("Expected net name for pin %s", cirPinGetName(pin));
        return false;
    }
    net = findOrCreateNet(netName);
    port = dbPortCreate(inst, mport);
    dbNetInsertPort(net, port);
    return true;
}

/*--------------------------------------------------------------------------------------------------
  Try to read the device attribute.  If we fail, back-up the string position, and return false.
--------------------------------------------------------------------------------------------------*/
static bool readAttr(
    dbInst inst,
    cirDevice device,
    cirAttr attr)
{
    dbAttr instAttr = dbAttrNull;
    utSym name, value;
    uint32 position = cirLinePosition;

    if(cirAttrNameVisible(attr)) {
        name = readIdentifier();
        if(name != utSymNull) {
            if(utSymGetLowerSym(name) != cirAttrGetSym(attr)) {
                /* Out of order attributes seem to happen */
                attr = cirDeviceFindAttr(device, name);
            }
            if(attr != cirAttrNull) {
                if(cirAttrValueVisible(attr)) {
                    if(cirGetc() != '=') {
                        cirError("Expected value for attribute %s", utSymGetName(name));
                    } else {
                        value = readValue();
                        if(value == utSymNull) {
                            cirError("Expected value for attribute %s", utSymGetName(name));
                        } else {
                            instAttr = dbAttrCreate(name, value);
                        }
                    }
                } else {
                    if(cirPeekChar() == '=') {
                        cirError("Not Expecting value for attribute %s", utSymGetName(name));
                    } else {
                        instAttr = dbAttrCreate(cirAttrGetSym(attr), utSymNull);
                    }
                }
            }
        }
    } else {
        /* We just need to make sure that optional value-only parameters
           are legal identifiers */
        name = readIdentifier();
        cirLinePosition = position;
        if(!cirAttrOptional(attr) || name != utSymNull) {
            cirLinePosition = position;
            value = readValue();
            if(value != utSymNull) {
                instAttr = dbAttrCreate(cirAttrGetSym(attr), value);
            }
        }
    }
    if(instAttr == dbAttrNull) {
        cirLinePosition = position;
        return false;
    }
    dbAttrSetNextAttr(instAttr, dbInstGetAttr(inst));
    dbInstSetAttr(inst, instAttr);
    return true;
}

/*--------------------------------------------------------------------------------------------------
  Read the device attributes.
--------------------------------------------------------------------------------------------------*/
static bool readDeviceAttrs(
    cirDevice device,
    dbInst inst)
{
    cirAttr attr;

    cirForeachDeviceAttr(device, attr) {
        if(!readAttr(inst, device, attr) && !cirAttrOptional(attr)) {
            cirError("Expected '%s' attribute", cirAttrGetName(attr));
            return false;
        }
    } cirEndDeviceAttr;
    return true;
}

/*--------------------------------------------------------------------------------------------------
  Instantiate a device.
--------------------------------------------------------------------------------------------------*/
static bool executeDevice(
    char type)
{
    cirDevice device;
    cirPin pin;
    dbNetlist netlist;
    dbInst inst;
    utSym sym, instName;
    char name[2];

    name[0] = type;
    name[1] = '\0';
    sym = utSymCreate(name);
    device = cirRootFindDevice(cirTheRoot, utSymGetLowerSym(sym));
    if(device == cirDeviceNull) {
        cirError("Unknown device type '%c'", type);
        return false;
    }
    netlist = cirDeviceGetInternalNetlist(device);
    cirNetlistSetUsed(netlist, true);
    instName = readIdentifier();
    if(instName == utSymNull) {
        cirError("Unable to read instance name");
        return false;
    }
    inst = dbInstCreate(cirCurrentNetlist, instName, netlist);
    cirForeachDevicePin(device, pin) {
        if(!readPin(pin, inst)) {
            return false;
        }
    } dbEndInstPort;
    if(!readDeviceAttrs(device, inst)) {
        return false;
    }
    return true;
}

/*--------------------------------------------------------------------------------------------------
  Execute the SPICE line.
--------------------------------------------------------------------------------------------------*/
static bool executeLine(void)
{
    char c = skipSpace();

    if(c == '*') {
        return true; /* Just a comment */
    }
    if(c == '.') {
        cirLinePosition++; /* Skip past the first character */
        return executeDirective();
    }
    if(cirSkipSubcircuit) {
        return true;
    }
    c = tolower(c);
    if(c < 'a' || c > 'z') {
        cirError("Unrecognized character '%c'", c);
        return false;
    }
    if(tolower(c) == 'x') {
        if(cirCurrentNetlist == dbNetlistNull) {
            cirError("Instance statements outside subckt blocks are ignored");
            return true;
        }
        /* Skip past the 'X' character, but only if the next is a legal identifier character */
        c = cirGetc();
        if(isdigit(cirPeekChar())) {
            cirUngetc();
        }
        return executeInstance();
    }
    if(cirCurrentNetlist == dbNetlistNull) {
        cirError("Device statements outside subckt blocks are ignored");
        return true;
    }
    return executeDevice(c);
}

/*--------------------------------------------------------------------------------------------------
  Read a spice file.
--------------------------------------------------------------------------------------------------*/
static bool readSpice(void)
{
    dbNetlist netlist;

    skipLine(); /* The first line is a comment */
    while(readLine()) {
        if(!executeLine()) {
            return false;
        }
    }

    dbForeachDesignNetlist(cirCurrentDesign, netlist) {
        if(dbNetlistGetType(netlist) == DB_UNDEFINED) {
            utWarning("Undefined subcircuit %s", utSymGetName(dbNetlistGetSym(netlist)));
        }
    } dbEndDesignNetlist;

    return true;
}

/*--------------------------------------------------------------------------------------------------
  Read in a SPICE design.
--------------------------------------------------------------------------------------------------*/
dbDesign cirReadDesign(
    char *designName,
    char *fileName,
    dbDesign libDesign)
{
    dbDesign design = dbDesignNull;
    utSym designSym = utSymCreate(designName);

    utLogMessage("Reading SPICE file %s", fileName);
    cirFileName = utNewA(char, strlen(fileName) + 1);
    cirMaxLineLength = 42;
    cirCheckParameters = true;
    cirCurrentLibrary = libDesign;
    cirLine = utNewA(char, cirMaxLineLength);
    strcpy(cirFileName, fileName);
    initKeywords();
    cirSkipSubcircuit = false;
    if(!utSetjmp()) {
        cirStart(dbDesignNull, dbSpiceTarget);
        if(!cirBuildDevices()) {
            utWarning("Could not build SPICE device configuration data");
        } else {
            cirFile = fopen(fileName, "r");
            if(cirFile == NULL) {
                utWarning("Could not open file %s", cirFileName);
            } else {
                cirCurrentDesign = dbRootFindDesign(dbTheRoot, designSym);
                if(cirCurrentDesign == dbDesignNull) {
                    cirCurrentDesign = dbDesignCreate(designSym, libDesign);
                }
                buildDeviceNetlists();
                cirLineNum = 0;
                cirCurrentNetlist = dbNetlistNull;
                cirLastNetlist = dbNetlistNull;
                if(readSpice() || cirLastNetlist == dbNetlistNull) {
                    freeUnusedDeviceNetlists();
                    design = cirCurrentDesign;
                    dbDesignSetRootNetlist(design, cirLastNetlist);
                } else {
                    dbDesignDestroy(cirCurrentDesign);
                }
            }
        }
    }
    cirStop();
    utUnsetjmp();
    utFree(cirFileName);
    utFree(cirLine);
    return design;
}

