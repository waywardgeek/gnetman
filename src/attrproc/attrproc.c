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
#include "db.h"
#include "at.h"

#define AT_SEP_CHARS " \t\r\n"

atRoot atTheRoot;
static utSym atSpiceTypeSym;

/*--------------------------------------------------------------------------------------------------
  Dump the context to the log file for debugging.
--------------------------------------------------------------------------------------------------*/
void atDumpContext(
    atContext context)
{
    atAttr attr;

    if(context == atContextNull) {
        utDebug("Context NULL\n");
        return;
    }
    utDebug("Context 0x%x\n", context - 0);
    atForeachContextAttr(context, attr) {
        utDebug("    attr %s=%g\n", utSymGetName(atAttrGetSym(attr)), atAttrGetValue(attr));
    } atEndContextAttr;
}

/*--------------------------------------------------------------------------------------------------
  Allocate memory for the attribute processor module.
--------------------------------------------------------------------------------------------------*/
void atStart(
    dbDesign design)
{
    atTheRoot = atRootAlloc();
    atSpiceTypeSym = utSymCreate("spicetype");
}

/*--------------------------------------------------------------------------------------------------
  Create a device.
--------------------------------------------------------------------------------------------------*/
atDevice atDeviceCreate(
    utSym deviceType)
{
    atDevice device = atDeviceAlloc();

    atDeviceSetSym(device, utSymGetLowerSym(deviceType));
    atRootInsertDevice(atTheRoot, device);
    return device;
}

/*--------------------------------------------------------------------------------------------------
  Create a pin.
--------------------------------------------------------------------------------------------------*/
atPin atPinCreate(
    atDevice device,
    utSym pinType)
{
    atPin pin = atPinAlloc();

    atPinSetSym(pin, utSymGetLowerSym(pinType));
    atDeviceInsertPin(device, pin);
    return pin;
}

/*--------------------------------------------------------------------------------------------------
  Free memory used in the short-cut module.
--------------------------------------------------------------------------------------------------*/
void atStop(
    dbDesign design)
{
    atRootDestroy(atTheRoot);
}

/*--------------------------------------------------------------------------------------------------
  Compute the value of the expression in the current context.
--------------------------------------------------------------------------------------------------*/
static double computeExpression(
    char *expression,
    atContext context,
    bool *passed)
{
    if(utSetjmp()) {
        *passed = false;
        return 0.0;
    }
    atLineNum = 1;
    atExpressionString = expression;
    atCurrentContext = context;
    atExpressionPassed = true;
    if(atparse()) {
        utUnsetjmp();
        return false;
    }
    utUnsetjmp();
    *passed = atExpressionPassed;
    return atCurrentValue;
}

/*--------------------------------------------------------------------------------------------------
  Build a new attribute object.
--------------------------------------------------------------------------------------------------*/
static atAttr atAttrCreate(
    atContext context,
    utSym name,
    double value)
{
    atAttr attr = atAttrAlloc();

    atAttrSetSym(attr, utSymGetLowerSym(name));
    atAttrSetValue(attr, value);
    atContextInsertAttr(context, attr);
    return attr;
}

/*--------------------------------------------------------------------------------------------------
  Find the context for evaluating expressions inside the netlist.  The context keeps track of
  parameters that are set through database attributes.  They get inherited SPICE-style.
--------------------------------------------------------------------------------------------------*/
static atContext findNetlistContext(
    dbNetlist netlist)
{
    atContext netlistContext = atContextAlloc();
    atAttr attr;
    dbAttr databaseAttr = dbNetlistGetAttr(netlist);
    double value;
    bool passed;
    
    databaseAttr = dbNetlistGetAttr(netlist);
    while(databaseAttr != dbAttrNull) {
        value = computeExpression(utSymGetName(dbAttrGetValue(databaseAttr)), atContextNull,
            &passed);
        if(passed) {
            attr = atAttrCreate(netlistContext, dbAttrGetName(databaseAttr), value);
        }
        databaseAttr = dbAttrGetNextAttr(databaseAttr);
    }
    return netlistContext;
}

/*--------------------------------------------------------------------------------------------------
  Find the context for evaluating expressions inside the instance.  The context keeps track of
  parameters that are set through database attributes.  They get inherited SPICE-style.
--------------------------------------------------------------------------------------------------*/
static atContext findInstContext(
    dbInst inst,
    atContext context)
{
    atContext instContext = atContextAlloc();
    atAttr attr;
    dbAttr databaseAttr = dbInstGetAttr(inst);
    double value;
    bool passed;
    
    while(databaseAttr != dbAttrNull) {
        value = computeExpression(utSymGetName(dbAttrGetValue(databaseAttr)), context, &passed);
        if(passed) {
            attr = atAttrCreate(instContext, dbAttrGetName(databaseAttr), value);
        }
        databaseAttr = dbAttrGetNextAttr(databaseAttr);
    }
    /* Set default attributes */
    databaseAttr = dbNetlistGetAttr(dbInstGetInternalNetlist(inst));
    while(databaseAttr != dbAttrNull) {
        if(atContextFindAttr(instContext,
                utSymGetLowerSym(dbAttrGetName(databaseAttr))) == atAttrNull) {
            value = computeExpression(utSymGetName(dbAttrGetValue(databaseAttr)), context, &passed);
            if(passed) {
                attr = atAttrCreate(instContext, dbAttrGetName(databaseAttr), value);
            }
        }
        databaseAttr = dbAttrGetNextAttr(databaseAttr);
    }
    return instContext;
}

static void setMportSum(dbMport mport, char *expression, atContext context, bool topLevel);
/*--------------------------------------------------------------------------------------------------
  Just return the gate area on the port.
--------------------------------------------------------------------------------------------------*/
static double findPortSum(
    dbPort port,
    char *expression,
    atContext context)
{
    atDevice device;
    atPin pin;
    dbMport mport = dbPortGetMport(port);
    dbInst inst = dbPortGetInst(port);
    dbNetlist internalNetlist = dbInstGetInternalNetlist(inst);
    dbAttr deviceAttrs = dbNetlistGetAttr(internalNetlist);
    atContext instContext;
    utSym deviceType;
    double value;
    bool passed;

    switch(dbNetlistGetType(internalNetlist)) {
    case DB_SUBCIRCUIT:
        if(dbInstGetAttr(inst) == dbAttrNull) {
            return atMportGetSum(mport);
        }
        instContext = findInstContext(inst, context);
        setMportSum(mport, expression, instContext, false);
        atContextDestroy(instContext);
        return atMportGetCurrentSum(mport);
    case DB_DEVICE:
        deviceType = dbFindAttrValue(deviceAttrs, atSpiceTypeSym);
        device = atRootFindDevice(atTheRoot, utSymGetLowerSym(deviceType));
        if(device == atDeviceNull) {
            return 0.0;
        }
        pin = atDeviceFindPin(device, utSymGetLowerSym(dbMportGetSym(mport)));
        if(pin == atPinNull) {
            return 0.0;
        }
        instContext = findInstContext(inst, context);
        value = computeExpression(expression, instContext, &passed);
        atContextDestroy(instContext);
        if(!passed) {
            utWarning("Could not evaluate expression on instance %s in netlist %s",
                dbInstGetUserName(inst), dbNetlistGetName(dbInstGetNetlist(inst)));
        }
        return value;
    case DB_POWER: case DB_FLAG:
        break;
    default:
        utExit("writeInst: Unknown netlist type");
    }
    return 0.0;
}

/*--------------------------------------------------------------------------------------------------
  Set the mport sum in the current context.  Annotate the value to a property on the mport.
--------------------------------------------------------------------------------------------------*/
static void setMportSum(
    dbMport mport,
    char *expression,
    atContext context,
    bool topLevel)
{
    dbPort internalPort = dbMportGetFlagPort(mport);
    dbNet net;
    dbPort port;
    double sum = 0.0;

    if(internalPort == dbPortNull) {
        return;
    }
    net = dbPortGetNet(internalPort);
    dbForeachNetPort(net, port) {
        sum += findPortSum(port, expression, context);
    } dbEndNetPort;
    if(topLevel) {
        atMportSetSum(mport, sum);
    } else {
        atMportSetCurrentSum(mport, sum);
    }
}

/*--------------------------------------------------------------------------------------------------
  Set the sum of the expression over all devices in the design.
--------------------------------------------------------------------------------------------------*/
static void setNetlistSums(
    dbNetlist netlist,
    char *expression)
{
    dbNetlist childNetlist;
    dbInst inst;
    dbMport mport;
    atContext context;

    dbNetlistSetVisited(netlist, true);
    dbForeachNetlistInst(netlist, inst) {
        childNetlist = dbInstGetInternalNetlist(inst);
        if(dbNetlistGetType(childNetlist) == DB_SUBCIRCUIT && !dbNetlistVisited(childNetlist)) {
            setNetlistSums(childNetlist, expression);
        }
    } dbEndNetlistInst;
    context = findNetlistContext(netlist);
    dbForeachNetlistMport(netlist, mport) {
        setMportSum(mport, expression, context, true);
    } dbEndNetlistMport;
    atContextDestroy(context);
}

/*--------------------------------------------------------------------------------------------------
  Report the sum of the expression over all devices in the design.
--------------------------------------------------------------------------------------------------*/
static void reportNetlistSums(
    dbNetlist netlist,
    char *expression)
{
    dbMport mport;

    setNetlistSums(netlist, expression);
    dbForeachNetlistMport(netlist, mport) {
        utDebug("portsum: %s %s %g\n", dbNetlistGetName(netlist),  dbMportGetName(mport),
            atMportGetSum(mport));
    } dbEndNetlistMport;
}

/*--------------------------------------------------------------------------------------------------
  Report the sum of the expression over all devices in the design.  Do this hierarchically,
  propogating attributes from instances down.
--------------------------------------------------------------------------------------------------*/
void atReportDesignSums(
    dbDesign design,
    utSym deviceType,
    utSym pinType,
    char *expression)
{
    dbNetlist netlist;
    atDevice device;

    atStart(design);
    device = atDeviceCreate(deviceType);
    atPinCreate(device, pinType);
    dbDesignClearNetlistVisitedFlags(design);
    dbForeachDesignNetlist(design, netlist) {
        if(!dbNetlistVisited(netlist) && dbNetlistGetType(netlist) == DB_SUBCIRCUIT) {
            reportNetlistSums(netlist, expression);
        }
    } dbEndDesignNetlist;
    atStop(design);
}

/*--------------------------------------------------------------------------------------------------
  Read the first token, but end at the first '#' char.
--------------------------------------------------------------------------------------------------*/
static char *readFirstToken(
    char *line)
{
    char *token = strtok(line, AT_SEP_CHARS);

    if(token == NULL) {
        return NULL;
    }
    return token;
}

/*--------------------------------------------------------------------------------------------------
  Read the next token, but end at the first '#' char.
--------------------------------------------------------------------------------------------------*/
static char *readNextToken(void)
{
    char *token = strtok(NULL, AT_SEP_CHARS);

    if(token == NULL) {
        return NULL;
    }
    return token;
}

/*--------------------------------------------------------------------------------------------------
  Report the sum of the expression over all devices in the design.  Do this hierarchically,
  propogating attributes from instances down.  The pins string allows multiple pins to be
  specified, and should be a list of alternating device names and pin names, such as "M S M D".
--------------------------------------------------------------------------------------------------*/
void atReportDesignSumsOverPins(
    dbDesign design,
    char *expression,
    char *pins)
{
    dbNetlist netlist;
    atDevice device;
    atPin pin;
    utSym deviceType, pinType;
    char *word = readFirstToken(pins);

    atStart(design);
    while(word != NULL) {
        deviceType = utSymCreate(word);
        word = readNextToken();
        if(word == NULL) {
            utWarning("Expected device pin pairs, but got an odd number of words in pin string");
            atStop(design);
            return;
        }
        pinType = utSymCreate(word);
        device = atRootFindDevice(atTheRoot, utSymGetLowerSym(deviceType));
        if(device == atDeviceNull) {
            device = atDeviceCreate(deviceType);
        }
        pin = atDeviceFindPin(device, utSymGetLowerSym(pinType));
        if(pin != atPinNull) {
            utWarning("Duplicate pin %s.%s", utSymGetName(deviceType), utSymGetName(pinType));
            atStop(design);
            return;
        }
        atPinCreate(device, pinType);
        word = readNextToken();
    }
    dbDesignClearNetlistVisitedFlags(design);
    dbForeachDesignNetlist(design, netlist) {
        if(!dbNetlistVisited(netlist) && dbNetlistGetType(netlist) == DB_SUBCIRCUIT) {
            reportNetlistSums(netlist, expression);
        }
    } dbEndDesignNetlist;
    atStop(design);
}
