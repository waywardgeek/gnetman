/*
 * Copyright (C) 2006 ViASIC
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
   This module writes PCB netlists.  Note: it had better be flattened first!
--------------------------------------------------------------------------------------------------*/

#include <stdarg.h>
#include <string.h>
#include "htext.h"
#include "pcb.h"

/*--------------------------------------------------------------------------------------------------
  Generate the port's name, in netname-portname format.
--------------------------------------------------------------------------------------------------*/
static char *getPortName(
    dbPort port)
{
    return utSprintf("%s-%s", dbInstGetUserName(dbPortGetInst(port)),
            dbMportGetName(dbPortGetMport(port)));
}

/*--------------------------------------------------------------------------------------------------
  Write the net.
--------------------------------------------------------------------------------------------------*/
static void writeNet(
    dbNet net)
{
    dbPort port;
    bool firstPort = true;

    fprintf(pcbFile, "%s\t", dbNetGetName(net));
    dbForeachNetPort(net, port) {
        if(dbInstGetType(dbPortGetInst(port)) != DB_FLAG) {
            if(!firstPort) {
                fputc(' ', pcbFile);
            }
            firstPort = false;
            fputs(getPortName(port), pcbFile);
        }
    } dbEndNetPort;
    fputc('\n', pcbFile);
}

/*--------------------------------------------------------------------------------------------------
  Write the nets in the netlist.
--------------------------------------------------------------------------------------------------*/
static void writeNets(
    dbNetlist netlist)
{
    dbNet net;

    dbForeachNetlistNet(netlist, net) {
        writeNet(net);
    } dbEndNetlistNet;
}

/*--------------------------------------------------------------------------------------------------
  Write a PCB netlist.  Since the format only supports flat designs, only the top level is written.
--------------------------------------------------------------------------------------------------*/
bool pcbWriteDesign(
    dbDesign design,
    char *fileName)
{
    utLogMessage("Writing PCB file %s", fileName);
    pcbFile = fopen(fileName, "w");
    if(!pcbFile) {
        return false;
    }
    writeNets(dbDesignGetRootNetlist(design));
    fclose(pcbFile);
    return true;
}

/*--------------------------------------------------------------------------------------------------
  Write a PCB netlist.
--------------------------------------------------------------------------------------------------*/
bool pcbWriteNetlist(
    dbNetlist netlist,
    char *fileName)
{
    utLogMessage("Writing PCB file %s", fileName);
    pcbFile = fopen(fileName, "w");
    if(!pcbFile) {
        return false;
    }
    writeNets(netlist);
    fclose(pcbFile);
    return true;
}
