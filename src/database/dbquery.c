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
  Query routings.
--------------------------------------------------------------------------------------------------*/
#include <string.h>
#include "db.h"

/*--------------------------------------------------------------------------------------------------
  Find the port on the inst from it's owning mport.
--------------------------------------------------------------------------------------------------*/
dbPort dbFindPortFromInstMport(
    dbInst inst,
    dbMport mport)
{
    dbPort port;

    dbForeachInstPort(inst, port) {
        if(dbPortGetMport(port) == mport) {
            return port;
        }
    } dbEndInstPort;
    return dbPortNull;
}

/*--------------------------------------------------------------------------------------------------
  Find the value in the attribute list.
--------------------------------------------------------------------------------------------------*/
utSym dbFindAttrValue(
    dbAttr attr,
    utSym name)
{
    if(attr == dbAttrNull) {
        return utSymNull;
    }
    if(dbAttrGetName(attr) == name) {
        return dbAttrGetValue(attr);
    }
    return dbFindAttrValue(dbAttrGetNextAttr(attr), name);
}

/*--------------------------------------------------------------------------------------------------
  Find the attrubute in the attribute list.
--------------------------------------------------------------------------------------------------*/
dbAttr dbFindAttr(
    dbAttr attr,
    utSym name)
{
    if(attr == dbAttrNull) {
        return dbAttrNull;
    }
    if(dbAttrGetName(attr) == name) {
        return attr;
    }
    return dbFindAttr(dbAttrGetNextAttr(attr), name);
}

/*--------------------------------------------------------------------------------------------------
  Find the attrubute in the attribute list.
--------------------------------------------------------------------------------------------------*/
dbAttr dbFindAttrNoCase(
    dbAttr attr,
    utSym name)
{
    utSym lowerName = utSymGetLowerSym(name);

    if(attr == dbAttrNull) {
        return dbAttrNull;
    }
    if(utSymGetLowerSym(dbAttrGetName(attr)) == lowerName) {
        return attr;
    }
    return dbFindAttrNoCase(dbAttrGetNextAttr(attr), name);
}

/*--------------------------------------------------------------------------------------------------
  Find the spice simulator type from the name.
--------------------------------------------------------------------------------------------------*/
dbSpiceTargetType dbFindSpiceTargetFromName(
    char *name)
{
    dbDevspec devspec = dbRootFindDevspec(dbTheRoot, utSymCreate(name));

    if(devspec == dbDevspecNull) {
        return DB_NULL_SPICE_TYPE;
    }
    return dbDevspecGetType(devspec);
}

/*--------------------------------------------------------------------------------------------------
  Find the device spec from it's type.
--------------------------------------------------------------------------------------------------*/
dbDevspec dbFindDevspecFromType(
    dbSpiceTargetType type)
{
    dbDevspec devspec;

    dbForeachRootDevspec(dbTheRoot, devspec) {
        if(dbDevspecGetType(devspec) == type) {
            return devspec;
        }
    } dbEndRootDevspec;
    return dbDevspecNull;
}

/*--------------------------------------------------------------------------------------------------
  Find the spice simulator type from the name.
--------------------------------------------------------------------------------------------------*/
char *dbGetSpiceTargetName(
    dbSpiceTargetType type)
{
    dbDevspec devspec = dbFindDevspecFromType(type);

    return utSymGetName(dbDevspecGetSym(devspec));
}

/*--------------------------------------------------------------------------------------------------
  Find the dbNetlistFormat specified by the string.
--------------------------------------------------------------------------------------------------*/
dbNetlistFormat dbFindNetlistFormat(
    char *format)
{
    if(!strcasecmp(format, "verilog")) {
        return DB_VERILOG;
    }
    if(!strcasecmp(format, "spice")) {
        return DB_SPICE;
    }
    if(!strcasecmp(format, "schematic")) {
        return DB_SCHEMATIC;
    }
    if(!strcasecmp(format, "pcb")) {
        return DB_PCB;
    }
    return DB_NOFORMAT;
}

/*--------------------------------------------------------------------------------------------------
  Find the current devspec object from dbSpiceTarget.
--------------------------------------------------------------------------------------------------*/
dbDevspec dbFindCurrentDevspec(void)
{
    return dbFindDevspecFromType(dbSpiceTarget);
}

