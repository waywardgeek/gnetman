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
  Constructors
--------------------------------------------------------------------------------------------------*/
#include "sch.h"

/*--------------------------------------------------------------------------------------------------
  Create an attribute.
--------------------------------------------------------------------------------------------------*/
schAttr schAttrCreate(
    int32 x,
    int32 y,
    utSym name,
    utSym value,
    bool visible,
    bool showName,
    bool showValue,
    uint8 color,
    uint8 size,
    uint8 angle,
    uint8 alignment)
{
    schAttr attr = schAttrAlloc();
    
    schAttrSetX(attr, x); 
    schAttrSetY(attr, y); 
    schAttrSetName(attr, name); 
    schAttrSetValue(attr, value); 
    schAttrSetVisible(attr, visible);
    schAttrSetShowName(attr, showName);
    schAttrSetShowValue(attr, showValue);
    schAttrSetColor(attr, color);
    schAttrSetSize(attr, size);
    schAttrSetAngle(attr, angle);
    schAttrSetAlignment(attr, alignment);
    return attr;
}

/*--------------------------------------------------------------------------------------------------
  Create a wire.
--------------------------------------------------------------------------------------------------*/
schWire schWireCreate(
    schSchem schem,
    int32 x1,
    int32 y1,
    int32 x2,
    int32 y2,
    bool isBus)
{
    schWire wire = schWireAlloc();

    schWireSetX1(wire, x1);
    schWireSetY1(wire, y1);
    schWireSetX2(wire, x2);
    schWireSetY2(wire, y2);
    schWireSetBus(wire, isBus);
    schSchemInsertWire(schem, wire);
    return wire;
}

/*--------------------------------------------------------------------------------------------------
  Create a component.
--------------------------------------------------------------------------------------------------*/
schComp schCompCreate(
    schSchem schem,
    utSym name,
    utSym symbolName,
    int32 x,
    int32 y,
    bool selectable,
    uint16 angle,
    bool mirror)
{
    schComp comp;
    uint32 left = 0, right = 0;
    bool isArray = false;

    if(name != utSymNull) {
        comp = schSchemFindComp(schem, name);
        if(comp != schCompNull) {
            utExit("schCompCreate: component %s already defined", utSymGetName(name));
        }
        isArray = dbNameHasRange(utSymGetName(name), &left, &right);
    }
    comp = schCompAlloc();
    schCompSetSym(comp, name);
    schCompSetSymbolName(comp, symbolName);
    schCompSetX(comp, x);
    schCompSetY(comp, y);
    schCompSetSelectable(comp, selectable);
    schCompSetAngle(comp, angle);
    schCompSetMirror(comp, mirror);
    schCompSetArray(comp, isArray);
    schCompSetLeft(comp, left);
    schCompSetRight(comp, right);
    schSchemInsertComp(schem, comp);
    return comp;
}

/*--------------------------------------------------------------------------------------------------
  Create an mpin.
--------------------------------------------------------------------------------------------------*/
schMpin schMpinCreate(
    schSymbol symbol,
    utSym name,
    schComp flagComp,
    dbMportType type,
    uint32 sequence,
    int32 x,
    int32 y,
    bool isBus,
    uint32 left,
    uint32 right)
{
    schMpin mpin = schMpinAlloc();

    schMpinSetSym(mpin, name);
    schMpinSetType(mpin, type);
    schMpinSetSequence(mpin, sequence);
    schMpinSetX(mpin, x);
    schMpinSetY(mpin, y);
    schMpinSetBus(mpin, isBus);
    schMpinSetLeft(mpin, left);
    schMpinSetRight(mpin, right);
    schSymbolAppendMpin(symbol, mpin);
    schMpinSetFlagComp(mpin, flagComp);
    if(flagComp != schCompNull) {
        schCompSetMpin(flagComp, mpin);
    }
    return mpin;
}

/*--------------------------------------------------------------------------------------------------
  Create a schematic.
--------------------------------------------------------------------------------------------------*/
schSchem schSchemCreate(
    utSym name,
    utSym path)
{
    schSchem schem = schSchemAlloc();

    schSchemSetSym(schem, name);
    schRootInsertSchem(schTheRoot, schem);
    schSchemSetPath(schem, path);
    return schem;
}

/*--------------------------------------------------------------------------------------------------
  Create a symbol.
--------------------------------------------------------------------------------------------------*/
schSymbol schSymbolCreate(
    utSym name,
    dbNetlistType type,
    utSym path)
{
    schSymbol symbol = schSymbolAlloc();

    schSymbolSetSym(symbol, name);
    schSymbolSetType(symbol, type);
    schRootInsertSymbol(schTheRoot, symbol);
    schSymbolSetPath(symbol, path);
    return symbol;
}

/*--------------------------------------------------------------------------------------------------
  Translate the point's coordinates as follows:
    1. reflect about the y-axis if reflect is set
    2. rotate about origin counter clockwise rotation*90 degrees
    3. move the point by the x and y offsets
--------------------------------------------------------------------------------------------------*/
static utPoint translatePoint(
    utPoint point,
    utTranslation translation)
{
    uint8 rotation = 0x3 & utTranslationGetRotation(translation);
    int32 xOffset = utTranslationGetX(translation);
    int32 yOffset = utTranslationGetY(translation);

    if(utTranslationReflect(translation)) {
        utPointSetX(point, -utPointGetX(point));
    }
    while(rotation > 0) {
        point = utMakePoint(-utPointGetY(point), utPointGetX(point));
        rotation--;
    }
    utPointSetX(point, utPointGetX(point) + xOffset);
    utPointSetY(point, utPointGetY(point) + yOffset);
    return point;
}

/*--------------------------------------------------------------------------------------------------
  Create a pin.  The X and Y coordinates on the pin are absolute coordinates.
--------------------------------------------------------------------------------------------------*/
schPin schPinCreate(
    schComp comp,
    schMpin mpin)
{
    schPin pin = schPinAlloc();
    int32 x = schCompGetX(comp);
    int32 y = schCompGetY(comp);
    uint8 rotation = (uint8)(schCompGetAngle(comp)/90);
    utTranslation translation = utMakeTranslation(schCompMirror(comp), rotation, x, y);
    utPoint point = utMakePoint(schMpinGetX(mpin), schMpinGetY(mpin));

    point = translatePoint(point, translation);
    schPinSetX(pin, utPointGetX(point));
    schPinSetY(pin, utPointGetY(point));
    schCompAppendPin(comp, pin);
    schPinSetMpin(pin, mpin);
    return pin;
}

/*--------------------------------------------------------------------------------------------------
  Create a net.
--------------------------------------------------------------------------------------------------*/
schNet schNetCreate(
    schSchem schem,
    utSym netName,
    bool isBus)
{
    schNet net = schNetAlloc();

    schNetSetSym(net, netName);
    schNetSetBus(net, isBus);
    schSchemInsertNet(schem, net);
    return net;
}

/*--------------------------------------------------------------------------------------------------
  Create a connection between two wires.
--------------------------------------------------------------------------------------------------*/
schConn schConnCreate(
    schWire wire1,
    schWire wire2,
    int32 x,
    int32 y)
{
    schConn conn = schConnAlloc();

    schConnSetX(conn, x);
    schConnSetY(conn, y);
    schWireInsertLConn(wire1, conn);
    schWireInsertRConn(wire2, conn);
    return conn;
}

/*--------------------------------------------------------------------------------------------------
  Create a rectangle object.
--------------------------------------------------------------------------------------------------*/
schRect schRectCreate(
    schGraphic graphic,
    utBox box,
    uint8 color)
{
    schRect rect = schRectAlloc();

    schRectSetBox(rect, box);
    schGraphicInsertRect(graphic, rect);
    schRectSetColor(rect, color);
    return rect;
}

/*--------------------------------------------------------------------------------------------------
  Create a signal object.
--------------------------------------------------------------------------------------------------*/
schSignal schSignalCreate(
    schNet net,
    schBus bus,
    utSym sym)
{
    schSignal signal = schSignalAlloc();

    schSignalSetSym(signal, sym);
    schSignalSetBus(signal, bus);
    schNetAppendSignal(net, signal);
    return signal;
}

/*--------------------------------------------------------------------------------------------------
  Create a bus object.
--------------------------------------------------------------------------------------------------*/
schBus schBusCreate(
    schSchem schem,
    utSym name,
    uint32 left,
    uint32 right)
{
    schBus bus = schBusAlloc();

    schBusSetSym(bus, name);
    schBusSetLeft(bus, left);
    schBusSetRight(bus, right);
    schSchemInsertBus(schem, bus);
    return bus;
}

