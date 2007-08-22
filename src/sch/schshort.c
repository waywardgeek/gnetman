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

#include <string.h>
#include <stdlib.h>
#include "sch.h"

/*--------------------------------------------------------------------------------------------------
  Find the value in the attribute list.
--------------------------------------------------------------------------------------------------*/
utSym schFindAttrValue(
    schAttr attr,
    utSym name)
{
    if(attr == schAttrNull) {
        return utSymNull;
    }
    if(schAttrGetName(attr) == name) {
        return schAttrGetValue(attr);
    }
    return schFindAttrValue(schAttrGetNextAttr(attr), name);
}

/*--------------------------------------------------------------------------------------------------
  Create a unique net name in the schematic.
--------------------------------------------------------------------------------------------------*/
utSym schSchemCreateUniqueNetName(
    schSchem schem,
    char *name)
{
    utSym sym = utSymCreate(name);
    uint32 x = 0;

    if(schSchemFindNet(schem, sym) == schNetNull) {
        return sym;
    }
    do {
        sym = utSymCreateFormatted("%s%u", name, x);
        x++;
    } while(schSchemFindNet(schem, sym) != schNetNull);
    return sym;
}

/*--------------------------------------------------------------------------------------------------
  Create a unique comp name in the schematic.
--------------------------------------------------------------------------------------------------*/
utSym schSchemCreateUniqueCompName(
    schSchem schem,
    char *name)
{
    utSym sym = utSymCreate(name);
    uint32 x = 0;

    if(schSchemFindComp(schem, sym) == schCompNull) {
        return sym;
    }
    do {
        sym = utSymCreateFormatted("%s%u", name, x);
        x++;
    } while(schSchemFindComp(schem, sym) != schCompNull);
    return sym;
}

/*--------------------------------------------------------------------------------------------------
  Change the name of the net to the new symbol.
--------------------------------------------------------------------------------------------------*/
void schNetRename(
    schNet net,
    utSym newName)
{
    schSchem schem = schNetGetSchem(net);

    schSchemRemoveNet(schem, net);
    schNetSetSym(net, newName);
    schSchemInsertNet(schem, net);
}

/*--------------------------------------------------------------------------------------------------
  Find the box containing a wire.
--------------------------------------------------------------------------------------------------*/
utBox schWireFindBox(
    schWire wire)
{
    int32 x1 = schWireGetX1(wire);
    int32 y1 = schWireGetY1(wire);
    int32 x2 = schWireGetX2(wire);
    int32 y2 = schWireGetY2(wire);
    utBox box = utMakeBox(utMin(x1, x2), utMin(y1, y2), utMax(x1, x2), utMax(y1, y2));

    return box;
}

/*--------------------------------------------------------------------------------------------------
  Just return the component's name.
--------------------------------------------------------------------------------------------------*/
char *schCompGetUserName(
    schComp comp)
{
    schMpin mpin = schCompGetMpin(comp);
    utSym sym;

    if(mpin != schMpinNull) {
        return schMpinGetName(mpin);
    }
    sym = schCompGetSym(comp);
    if(sym != utSymNull) {
        return utSymGetName(schCompGetSym(comp));
    }
    return "<unnamed>";
}

/*--------------------------------------------------------------------------------------------------
  Just return the symbol's name.
--------------------------------------------------------------------------------------------------*/
dbMportType schFindFlagType(
    schComp comp)
{
    schSymbol symbol = schCompGetSymbol(comp);
    schMpin mpin = schSymbolGetFirstMpin(symbol);

    if(mpin == schMpinNull) {
        utExit("Flag %s has no pin", schSymbolGetName(symbol));
    }
    return dbInvertMportType(schMpinGetType(mpin));
}

/*--------------------------------------------------------------------------------------------------
  Determine if the pin type belongs on the right or left side of a symbol.
--------------------------------------------------------------------------------------------------*/
bool schPinTypeOnRight(
    dbMportType type)
{
    switch(type) {
    case DB_OUT:
    case DB_IO:
    case DB_OC:
    case DB_OE:
    case DB_PAS:
    case DB_TRI:
        return true;
    case DB_IN:
    case DB_TP:
    case DB_CLK:
    case DB_PWR:
        return false;
    default:
        utExit("schPinTypeOnRight: unknown pin type");
    }
    return false; /* Dummy return */
}

/*--------------------------------------------------------------------------------------------------
  Guess the width of the text.
--------------------------------------------------------------------------------------------------*/
uint32 schFindTextSpace(
    utSym name)
{
    return strlen(utSymGetName(name))*SCH_CHAR_WIDTH;
}

/*--------------------------------------------------------------------------------------------------
  Insert an attribute onto the symbol's list.
--------------------------------------------------------------------------------------------------*/
void schSymbolInsertAttr(
    schSymbol symbol,
    schAttr attr)
{
    schAttrSetNextAttr(attr, schSymbolGetAttr(symbol));
    schSymbolSetAttr(symbol, attr);
}

/*--------------------------------------------------------------------------------------------------
  Insert an attribute onto the mpin's list.
--------------------------------------------------------------------------------------------------*/
void schMpinInsertAttr(
    schMpin mpin,
    schAttr attr)
{
    schAttrSetNextAttr(attr, schMpinGetAttr(mpin));
    schMpinSetAttr(mpin, attr);
}

/*--------------------------------------------------------------------------------------------------
  Find the netlist type from the device.
--------------------------------------------------------------------------------------------------*/
dbNetlistType schSymbolFindType(
   schSymbol symbol)
{
    utSym device = schSymbolGetDevice(symbol);

    if(schSymbolGetSchem(symbol) != schSchemNull) {
        return DB_SUBCIRCUIT;
    }
    if(device == utSymNull) {
        return DB_DEVICE;
    }
    if(device == schFlagSym) {
        return DB_FLAG;
    }
    if(device == schPowerSym) {
        return DB_POWER;
    }
    return DB_DEVICE;
}

/*--------------------------------------------------------------------------------------------------
  Determine if the pin is a bus pin.  Flag pins have mpins that are not bus pins, but based on the
  component name, the pin can be a bus.  Non-bus mpins on instances arrays are bus pins if they
  attach to busses.
--------------------------------------------------------------------------------------------------*/
bool schPinIsBus(
    schPin pin)
{
    schComp comp = schPinGetComp(pin);
    schMpin mpin = schPinGetMpin(pin);
    schSymbol symbol = schMpinGetSymbol(mpin);
    schNet net;

    if(schSymbolGetType(symbol) == DB_FLAG) {
        return schCompArray(comp);
    }
    if(schMpinBus(mpin)) {
        return true;
    }
    if(schCompArray(comp)) {
        net = schPinGetNet(pin);
        if(net != schNetNull && schNetBus(schPinGetNet(pin))) {
            return true;
        }
    }
    return false;
}
