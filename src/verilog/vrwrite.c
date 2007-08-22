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
   This module writes Verilog gate level netlists.
--------------------------------------------------------------------------------------------------*/

#include <stdarg.h>
#include <string.h>
#include "htext.h"
#include "vr.h"

#define VR_WRAP_COLUMN 100

static htHtbl vrKeywordTable;

/*--------------------------------------------------------------------------------------------------
  Iinitialize the keyword hash table.
--------------------------------------------------------------------------------------------------*/
static void buildKeywordTable(void)
{
    vrKeywordTable = htHtblCreate();
    htHtblAddSym(vrKeywordTable, utSymCreate("always"));
    htHtblAddSym(vrKeywordTable, utSymCreate("and"));
    htHtblAddSym(vrKeywordTable, utSymCreate("assign"));
    htHtblAddSym(vrKeywordTable, utSymCreate("attribute"));
    htHtblAddSym(vrKeywordTable, utSymCreate("begin"));
    htHtblAddSym(vrKeywordTable, utSymCreate("buf"));
    htHtblAddSym(vrKeywordTable, utSymCreate("bufif0"));
    htHtblAddSym(vrKeywordTable, utSymCreate("bufif1"));
    htHtblAddSym(vrKeywordTable, utSymCreate("case"));
    htHtblAddSym(vrKeywordTable, utSymCreate("casex"));
    htHtblAddSym(vrKeywordTable, utSymCreate("casez"));
    htHtblAddSym(vrKeywordTable, utSymCreate("cmos"));
    htHtblAddSym(vrKeywordTable, utSymCreate("deassign"));
    htHtblAddSym(vrKeywordTable, utSymCreate("default"));
    htHtblAddSym(vrKeywordTable, utSymCreate("defparam"));
    htHtblAddSym(vrKeywordTable, utSymCreate("disable"));
    htHtblAddSym(vrKeywordTable, utSymCreate("edge"));
    htHtblAddSym(vrKeywordTable, utSymCreate("else"));
    htHtblAddSym(vrKeywordTable, utSymCreate("end"));
    htHtblAddSym(vrKeywordTable, utSymCreate("endattribute"));
    htHtblAddSym(vrKeywordTable, utSymCreate("endcase"));
    htHtblAddSym(vrKeywordTable, utSymCreate("endfunction endmodule"));
    htHtblAddSym(vrKeywordTable, utSymCreate("endprimitive"));
    htHtblAddSym(vrKeywordTable, utSymCreate("endspecify"));
    htHtblAddSym(vrKeywordTable, utSymCreate("endtable"));
    htHtblAddSym(vrKeywordTable, utSymCreate("endtask"));
    htHtblAddSym(vrKeywordTable, utSymCreate("event"));
    htHtblAddSym(vrKeywordTable, utSymCreate("for"));
    htHtblAddSym(vrKeywordTable, utSymCreate("force"));
    htHtblAddSym(vrKeywordTable, utSymCreate("forever"));
    htHtblAddSym(vrKeywordTable, utSymCreate("fork"));
    htHtblAddSym(vrKeywordTable, utSymCreate("function"));
    htHtblAddSym(vrKeywordTable, utSymCreate("highz0"));
    htHtblAddSym(vrKeywordTable, utSymCreate("highz1"));
    htHtblAddSym(vrKeywordTable, utSymCreate("if"));
    htHtblAddSym(vrKeywordTable, utSymCreate("ifnone"));
    htHtblAddSym(vrKeywordTable, utSymCreate("initial"));
    htHtblAddSym(vrKeywordTable, utSymCreate("inout"));
    htHtblAddSym(vrKeywordTable, utSymCreate("input"));
    htHtblAddSym(vrKeywordTable, utSymCreate("integer"));
    htHtblAddSym(vrKeywordTable, utSymCreate("join"));
    htHtblAddSym(vrKeywordTable, utSymCreate("medium"));
    htHtblAddSym(vrKeywordTable, utSymCreate("module large"));
    htHtblAddSym(vrKeywordTable, utSymCreate("macromodule"));
    htHtblAddSym(vrKeywordTable, utSymCreate("nand"));
    htHtblAddSym(vrKeywordTable, utSymCreate("negedge"));
    htHtblAddSym(vrKeywordTable, utSymCreate("nmos"));
    htHtblAddSym(vrKeywordTable, utSymCreate("nor"));
    htHtblAddSym(vrKeywordTable, utSymCreate("not"));
    htHtblAddSym(vrKeywordTable, utSymCreate("notif0"));
    htHtblAddSym(vrKeywordTable, utSymCreate("notif1"));
    htHtblAddSym(vrKeywordTable, utSymCreate("or"));
    htHtblAddSym(vrKeywordTable, utSymCreate("output"));
    htHtblAddSym(vrKeywordTable, utSymCreate("parameter"));
    htHtblAddSym(vrKeywordTable, utSymCreate("pmos"));
    htHtblAddSym(vrKeywordTable, utSymCreate("posedge"));
    htHtblAddSym(vrKeywordTable, utSymCreate("primitive"));
    htHtblAddSym(vrKeywordTable, utSymCreate("pull0"));
    htHtblAddSym(vrKeywordTable, utSymCreate("pull1"));
    htHtblAddSym(vrKeywordTable, utSymCreate("pulldown"));
    htHtblAddSym(vrKeywordTable, utSymCreate("pullup"));
    htHtblAddSym(vrKeywordTable, utSymCreate("rcmos"));
    htHtblAddSym(vrKeywordTable, utSymCreate("real"));
    htHtblAddSym(vrKeywordTable, utSymCreate("realtime reg"));
    htHtblAddSym(vrKeywordTable, utSymCreate("release"));
    htHtblAddSym(vrKeywordTable, utSymCreate("repeat"));
    htHtblAddSym(vrKeywordTable, utSymCreate("rnmos"));
    htHtblAddSym(vrKeywordTable, utSymCreate("rpmos"));
    htHtblAddSym(vrKeywordTable, utSymCreate("rtran"));
    htHtblAddSym(vrKeywordTable, utSymCreate("rtranif0"));
    htHtblAddSym(vrKeywordTable, utSymCreate("rtranif1"));
    htHtblAddSym(vrKeywordTable, utSymCreate("scalared"));
    htHtblAddSym(vrKeywordTable, utSymCreate("signed"));
    htHtblAddSym(vrKeywordTable, utSymCreate("small"));
    htHtblAddSym(vrKeywordTable, utSymCreate("specify"));
    htHtblAddSym(vrKeywordTable, utSymCreate("specparam"));
    htHtblAddSym(vrKeywordTable, utSymCreate("strength"));
    htHtblAddSym(vrKeywordTable, utSymCreate("strong0"));
    htHtblAddSym(vrKeywordTable, utSymCreate("strong1"));
    htHtblAddSym(vrKeywordTable, utSymCreate("supply0"));
    htHtblAddSym(vrKeywordTable, utSymCreate("supply1"));
    htHtblAddSym(vrKeywordTable, utSymCreate("table"));
    htHtblAddSym(vrKeywordTable, utSymCreate("task"));
    htHtblAddSym(vrKeywordTable, utSymCreate("time"));
    htHtblAddSym(vrKeywordTable, utSymCreate("tran tranif0"));
    htHtblAddSym(vrKeywordTable, utSymCreate("tranif1"));
    htHtblAddSym(vrKeywordTable, utSymCreate("tri"));
    htHtblAddSym(vrKeywordTable, utSymCreate("tri0"));
    htHtblAddSym(vrKeywordTable, utSymCreate("tri1"));
    htHtblAddSym(vrKeywordTable, utSymCreate("triand"));
    htHtblAddSym(vrKeywordTable, utSymCreate("trior"));
    htHtblAddSym(vrKeywordTable, utSymCreate("trireg"));
    htHtblAddSym(vrKeywordTable, utSymCreate("unsigned"));
    htHtblAddSym(vrKeywordTable, utSymCreate("vectored"));
    htHtblAddSym(vrKeywordTable, utSymCreate("wait"));
    htHtblAddSym(vrKeywordTable, utSymCreate("wand"));
    htHtblAddSym(vrKeywordTable, utSymCreate("weak0"));
    htHtblAddSym(vrKeywordTable, utSymCreate("weak1"));
    htHtblAddSym(vrKeywordTable, utSymCreate("while"));
    htHtblAddSym(vrKeywordTable, utSymCreate("wire"));
    htHtblAddSym(vrKeywordTable, utSymCreate("wor"));
    htHtblAddSym(vrKeywordTable, utSymCreate("xnor"));
    htHtblAddSym(vrKeywordTable, utSymCreate("xor"));
}

/*--------------------------------------------------------------------------------------------------
  Write out the text to vrFile, just as fprintf, except print
  a new line if there is not enough space.
--------------------------------------------------------------------------------------------------*/
void vrPrint(
    char *newLineText,
    char *format,
    ...)
{
    char *buf;
    va_list ap;
    uint32 length;

    va_start(ap, format);
    buf = utVsprintf(format, ap);
    length = strlen(buf);
    if(length + vrLinePos > VR_WRAP_COLUMN) {
        vrLinePos = fprintf(vrFile, "\n%s", newLineText);
    }
    vrLinePos += fprintf(vrFile, "%s", buf);
}

/*--------------------------------------------------------------------------------------------------
  Call vrPrint, and print a new line.
--------------------------------------------------------------------------------------------------*/
void vrPrintLn(
    char *newLineText,
    char *format,
    ...)
{
    char *buf;
    va_list ap;
    uint32 length;

    va_start(ap, format);
    buf = utVsprintf(format, ap);
    length = strlen(buf);
    if(length + vrLinePos > VR_WRAP_COLUMN) {
        fprintf(vrFile, "\n%s", newLineText);
    }
    fprintf(vrFile, "%s\n", buf);
    vrLinePos = 0;
}

/*--------------------------------------------------------------------------------------------------
  Determine if the name has any non-Verilog identifier chars.
--------------------------------------------------------------------------------------------------*/
static bool nameHasIllegalChars(
    char *name)
{
    if(*name >= '0' && *name <= '9') {
        return true;
    }
    while(*name) {
        if(!(*name >= 'a' && *name <= 'z') &&
                 !(*name >= 'A' && *name <= 'Z') && *name != '_' &&
                 !(*name >= '0' && *name <= '9')) {
            return true;
        }
        name++;
    }
    return false;
}

/*--------------------------------------------------------------------------------------------------
  Detect if the string is a keyword.
--------------------------------------------------------------------------------------------------*/
static bool stringIsKeyword(
    char *string)
{
    if(htHtblLookupSym(vrKeywordTable, utSymCreate(string)) != utSymNull) {
        return true;
    }
    return false;
}

/*--------------------------------------------------------------------------------------------------
  Add a '\' if there are any non-Verilog characters in the name or if it is a keyword.
--------------------------------------------------------------------------------------------------*/
char *vrMunge(
    char *name)
{
    if(nameHasIllegalChars(name)) {
        if(*name == '\\') {
            return utSprintf("%s ", name);
        }
        return utSprintf("\\%s ", name);
    }
    if(stringIsKeyword(name)) {
        return utSprintf("\\%s ", name);
    }
    return name;
}

/*--------------------------------------------------------------------------------------------------
  Return a name for a direction..
--------------------------------------------------------------------------------------------------*/
static char *getDirectionName(
    dbDirection direction)
{
    switch(direction) {
    case DB_INPUT: return "input";
    case DB_OUTPUT: return "output";  
    case DB_INOUT: return "inout";
    default:
        utExit("getDirectionName: unknown direction");
    }
    return NULL; /* Dummy return */
}

/*--------------------------------------------------------------------------------------------------
  Write an mport parameter of a netlist.
--------------------------------------------------------------------------------------------------*/
static void writeMportParameter(
    dbMport mport)
{
    vrPrint("   ", "%s", vrMunge(dbMportGetName(mport)));
}

/*--------------------------------------------------------------------------------------------------
  Write an mbus parameter of a netlist.
--------------------------------------------------------------------------------------------------*/
static void writeMbusParameter(
    dbMbus mbus)
{
    vrPrint("   ", "%s", vrMunge(dbMbusGetName(mbus)));
}

/*--------------------------------------------------------------------------------------------------
  Write the mport parameters of a netlist.
--------------------------------------------------------------------------------------------------*/
static void writeMportParameters(
    dbNetlist netlist)
{
    dbMport mport;
    dbMbus pMbus = dbMbusNull, mbus;
    bool isFirst = true; 

    dbForeachNetlistMport(netlist, mport) {
        mbus = dbMportGetMbus(mport);
        if(mbus == dbMbusNull) {
            if(!isFirst) {
                vrPrint("   ", ", ");
            }
            writeMportParameter(mport);
        } else if(mbus != pMbus) {
            if(!isFirst) {
                vrPrint("   ", ", ");
            }
            writeMbusParameter(mbus);
            pMbus = mbus;
        }
        isFirst = false;
    } dbEndNetlistMport;
}

/*--------------------------------------------------------------------------------------------------
  Write a netlist of a design.
--------------------------------------------------------------------------------------------------*/
static void writeNetlistHeader(
    dbNetlist netlist)
{
    vrPrint("", "module %s (", vrMunge(dbNetlistGetName(netlist)));
    writeMportParameters(netlist);
    vrPrintLn("   ", ");");
}

/*--------------------------------------------------------------------------------------------------
  If a net is one or zero, return 1'b1 or 1'b0.
--------------------------------------------------------------------------------------------------*/
static char *vrMungeNetName(
    dbNet net)
{

    if(dbNetIsZeroOrOne(net)) {
        if(dbNetIsOne(net)) {
            return "1'b1";
        }
        if(dbNetIsZero(net)) {
            return "1'b0";
        }
    }
    return vrMunge(dbNetGetName(net));
}

/*--------------------------------------------------------------------------------------------------
  Find the proper verilog name for the mport.
--------------------------------------------------------------------------------------------------*/
static char *vrMungeMportName(
    dbMport mport)
{
    return vrMunge(dbMportGetName(mport));
}

/*--------------------------------------------------------------------------------------------------
  Write declarations for busses in the netlist.
--------------------------------------------------------------------------------------------------*/
static void writeMbusDeclarations(
    dbNetlist netlist,
    dbDirection direction)
{
    dbMbus mbus;
    uint32 left, right;

    dbForeachNetlistMbus(netlist, mbus) {
        left = dbMbusGetLeft(mbus);
        right = dbMbusGetRight(mbus);
        if(dbFindMportTypeDirection(dbMbusGetType(mbus)) == direction) {
            vrPrint("", "%s [%u:%u] %s",
                     getDirectionName(direction), left, right,
                     vrMunge(dbMbusGetName(mbus)));
            vrPrintLn("   ", ";");
        }
    } dbEndNetlistMbus;
}

/*--------------------------------------------------------------------------------------------------
  Write declarations for nets in the netlist not in busses.
--------------------------------------------------------------------------------------------------*/
static void writeMportDeclarations(
    dbNetlist netlist,
    dbDirection direction)
{
    dbMport mport;
    bool isFirst = true;

    dbForeachNetlistMport(netlist, mport) {
        if(dbMportGetMbus(mport) == dbMbusNull) {
            if(dbFindMportTypeDirection(dbMportGetType(mport)) == direction) {
                if(isFirst) {
                    vrPrint("", "%s ", getDirectionName(direction));
                } else {
                    vrPrint("   ", ", ");
                }
                isFirst = false;
                vrPrint("   ", "%s", vrMunge(dbMportGetName(mport)));
            }
        }
    } dbEndNetlistMport;
    if(!isFirst) {
        vrPrintLn("   ", ";");
    }
}

/*--------------------------------------------------------------------------------------------------
  Write declarations for busses in the netlist.
--------------------------------------------------------------------------------------------------*/
static void writeBusDeclarations(
    dbNetlist netlist)
{
    dbBus bus;
    dbMbus mbus;
    uint32 left, right;

    dbForeachNetlistBus(netlist, bus) {
        mbus = dbNetlistFindMbus(netlist, dbBusGetSym(bus));
        left = dbBusGetLeft(bus);
        right = dbBusGetRight(bus);
        if(mbus == dbMbusNull) {
            vrPrint("", "wire [%u:%u] %s", left, right,
                     vrMunge(dbBusGetName(bus)));
            vrPrintLn("   ", ";");
        }
    } dbEndNetlistBus;
}

/*--------------------------------------------------------------------------------------------------
  Write declarations for nets in the netlist not in busses.
--------------------------------------------------------------------------------------------------*/
static void writeNetDeclarations(
    dbNetlist netlist)
{
    dbNet net;
    dbMport mport;
    bool isFirst = true;

    dbForeachNetlistNet(netlist, net) {
        if(dbNetGetBus(net) == dbBusNull && !dbNetIsZeroOrOne(net)) {
            mport = dbNetlistFindMport(netlist, dbNetGetSym(net));
            if(mport == dbMportNull) {
                if(isFirst) {
                    vrPrint("", "wire ");
                } else {
                    vrPrint("   ", ", ");
                }
                isFirst = false;
                vrPrint("   ", "%s", vrMungeNetName(net));
            }
        }
    } dbEndNetlistNet;
    if(!isFirst) {
        vrPrintLn("   ", ";");
    }
}

/*--------------------------------------------------------------------------------------------------
  Write the declarations of a netlist.
--------------------------------------------------------------------------------------------------*/
static void writeNetlistDeclarations(
    dbNetlist netlist)
{
    writeMbusDeclarations(netlist, DB_INPUT);
    writeMportDeclarations(netlist, DB_INPUT);
    writeMbusDeclarations(netlist, DB_OUTPUT);
    writeMportDeclarations(netlist, DB_OUTPUT);
    writeMbusDeclarations(netlist, DB_INOUT);
    writeMportDeclarations(netlist, DB_INOUT);
    writeBusDeclarations(netlist);
    writeNetDeclarations(netlist);
    vrPrintLn("", "");
}

/*--------------------------------------------------------------------------------------------------
  Just print out the name of the connection.
--------------------------------------------------------------------------------------------------*/
static void writeConnectionName(
    dbNet net,
    uint32 leftIndex,
    uint32 rightIndex)
{
    dbBus bus = dbNetGetBus(net);
    uint32 left, right;

    if(bus != dbBusNull) {
        left = dbBusGetLeft(bus);
        right = dbBusGetRight(bus);
        if(left < right) {
            leftIndex += left;
            rightIndex += left;
        } else {
            leftIndex = right + (left - right) - leftIndex;
            rightIndex = right + (left - right) - rightIndex;
        }
        vrPrint("      ", "%s[%u:%u]", vrMunge(dbBusGetName(bus)), leftIndex, rightIndex);
    } else {
        vrPrint("      ", "%s", vrMungeNetName(net));
    }
}

/*--------------------------------------------------------------------------------------------------
  Write a bus-port declaration for a bus connecting to an inst.
--------------------------------------------------------------------------------------------------*/
static void writeBusConnection(
    dbInst inst,
    dbMbus mbus)
{
    dbMport mport;
    dbPort port;
    dbNet net, pNet = dbNetNull;
    dbBus bus, pBus = dbBusNull;
    uint32 leftIndex = 0, rightIndex = 0;
    bool firstBus = true;

    dbForeachMbusMport(mbus, mport) {
        port = dbFindPortFromInstMport(inst, mport);
        net = dbPortGetNet(port);
        utAssert(net != dbNetNull);
        bus = dbNetGetBus(net);
        if(pNet != dbNetNull && (pBus == dbBusNull || pBus != bus ||
                utAbs((int32)dbNetGetBusIndex(net) - (int32)rightIndex) != 1)) {
            if(firstBus) {
                vrPrint("      ", "{");
            } else {
                vrPrint("      ", ", ");
            }
            firstBus = false;
            writeConnectionName(pNet, leftIndex, rightIndex);
            leftIndex = dbNetGetBusIndex(net);
            rightIndex = leftIndex;
        } else {
            rightIndex = dbNetGetBusIndex(net);
            if(pNet == dbNetNull) {
                leftIndex = rightIndex;
            }
        }
        pBus = bus;
        pNet = net;
    } dbEndMbusMport;
    if(!firstBus) {
        vrPrint("      ", ", ");
    }
    writeConnectionName(pNet, leftIndex, rightIndex);
    if(!firstBus) {
        vrPrint("      ", "}");
    }
}

/*--------------------------------------------------------------------------------------------------
  Write an instance declaration.
--------------------------------------------------------------------------------------------------*/
static void writeNetlistInst(
    dbInst inst)
{
    dbNetlist internalNetlist = dbInstGetInternalNetlist(inst);
    dbMport mport;
    dbPort port;
    dbNet net;
    dbMbus pMbus = dbMbusNull, mbus;
    bool isFirst = true; 

    vrPrint("", "   %s ", vrMunge(dbNetlistGetName(internalNetlist)));
    vrPrint("      ", "%s(", vrMunge(dbInstGetUserName(inst)));
    dbForeachNetlistMport(internalNetlist, mport) {
        port = dbFindPortFromInstMport(inst, mport);
        net = dbPortGetNet(port);
        /* Assumes all mbus bits are connected */
        if(net != dbNetNull) {
            mbus = dbMportGetMbus(mport);
            if(mbus == dbMbusNull || mbus != pMbus) {
                if(!isFirst) {
                    vrPrint("      ", ", ");
                }
                isFirst = false;
                if(mbus == dbMbusNull) {
                    vrPrint("      ", ".%s(", vrMunge(dbMportGetName(mport)));
                    vrPrint("      ", "%s", vrMungeNetName(net));
                } else {
                    vrPrint("      ", ".%s(", vrMunge(dbMbusGetName(mbus)));
                    writeBusConnection(inst, mbus);
                    pMbus = mbus;
                }
                vrPrint("      ", ")");
            }
        }
    } dbEndNetlistMport;
    vrPrintLn("      ", ");");
}

/*--------------------------------------------------------------------------------------------------
  Write the instances of a netlist.
--------------------------------------------------------------------------------------------------*/
static void writeNetlistInstances(
    dbNetlist netlist)
{
    dbInst inst;

    dbForeachNetlistInst(netlist, inst) {
        if(dbInstGetType(inst) != DB_FLAG) {
            if(!dbInstIsGraphical(inst)) {
                writeNetlistInst(inst);
            }
        }
    } dbEndNetlistInst;
}

/*--------------------------------------------------------------------------------------------------
  Write assignment statements for cases where mports are not attached to
  nets by the same name.
--------------------------------------------------------------------------------------------------*/
static void writeMportAssignments(
    dbNetlist netlist)
{
    dbMport mport;
    dbPort port;
    dbNet net;

    dbForeachNetlistMport(netlist, mport) {
        port = dbMportGetFlagPort(mport);
        if(port != dbPortNull) {
            net = dbPortGetNet(dbMportGetFlagPort(mport));
            if(dbNetGetSym(net) != dbMportGetSym(mport)) {
                if(dbFindMportTypeDirection(dbMportGetType(mport)) == DB_INPUT) {
                    vrPrintLn("      ", "   assign %s = %s;",
                        vrMungeNetName(net), vrMungeMportName(mport));
                } else {
                    vrPrintLn("      ", "   assign %s = %s;",
                        vrMungeMportName(mport), vrMungeNetName(net));
                }
            }
        }
    } dbEndNetlistMport;
}

/*--------------------------------------------------------------------------------------------------
  Write a netlist of a design.
--------------------------------------------------------------------------------------------------*/
static void writeNetlist(
    dbNetlist netlist)
{
    dbNetlistType type = dbNetlistGetType(netlist);

    if(type == DB_FLAG || type == DB_DEVICE) {
        return; /* Don't write out flag netlists or device netlists */
    }
    writeNetlistHeader(netlist);
    writeNetlistDeclarations(netlist);
    writeNetlistInstances(netlist);
    writeMportAssignments(netlist);
    vrPrintLn("", "endmodule\n");
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
  Write a verilog gate level design.
--------------------------------------------------------------------------------------------------*/
bool vrWriteDesign(
    dbDesign design,
    char *fileName,
    bool wholeLibrary)
{
    utLogMessage("Writing Verilog file %s", fileName);
    vrFile = fopen(fileName, "w");
    if(!vrFile) {
        return false;
    }
    vrLinePos = 0;
    buildKeywordTable();
    writeNetlists(design, wholeLibrary);
    htHtblDestroy(vrKeywordTable);
    fclose(vrFile);
    return true;
}

/*--------------------------------------------------------------------------------------------------
  Write a verilog netlist.
--------------------------------------------------------------------------------------------------*/
bool vrWriteNetlist(
    dbNetlist netlist,
    char *fileName)
{
    utLogMessage("Writing Verilog file %s", fileName);
    vrFile = fopen(fileName, "w");
    if(!vrFile) {
        return false;
    }
    vrLinePos = 0;
    buildKeywordTable();
    writeNetlist(netlist);
    htHtblDestroy(vrKeywordTable);
    fclose(vrFile);
    return true;
}
