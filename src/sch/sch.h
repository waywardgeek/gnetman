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

#include "db.h"
#include "schdatabase.h"
#include "schext.h"
#include "schtypes.h"
#include "schcolors.h"

/* Top level functions */
schSchem schReadSchem(char *fileName, bool loadSubSchems);
schSymbol schReadSymbol(char *fileName, bool loadSubSchems);
void schStartReader(void);
void schStopReader(void);
void schStartSchem(void);
void schStopSchem(void);
bool schSchemPostProcess(schSchem schem, bool loadSubSchems);
bool schSymbolPostProcess(schSymbol symbol, bool loadSubSchems);
bool schBuildNetlists(dbDesign design, dbDesign libDesign, schSchem rootSchem);
void schSchemCreateDefaultSymbol(schSchem schem);
bool schWriteSymbol(char *fileName, schSymbol symbol);
void schGensymStart(void);
void schGensymStop(void);
void schStart(void);
void schStop(void);

/* Constructors */
schAttr schAttrCreate(int32 x, int32 y, utSym name, utSym value,
    bool visible, bool nameVisible, bool valueVisible, uint8 color, uint8 size, uint8 angle, uint8 alignment);
schWire schWireCreate(schSchem schem, int32 x1, int32 y1, int32 x2, int32 y2, bool isBus);
schComp schCompCreate(schSchem schem, utSym name, utSym symbolName, int32 x, int32 y, bool selectable,
    uint16 angle, bool mirror);
schMpin schMpinCreate(schSymbol symbol, utSym name, schComp flagComp, dbMportType type, uint32 sequence,
    int32 x, int32 y, bool isBus, uint32 left, uint32 right);
schPin schPinCreate(schComp comp, schMpin mpin);
schSchem schSchemCreate(utSym name, utSym path);
schSymbol schSymbolCreate(utSym name, dbNetlistType type, utSym path);
schNet schNetCreate(schSchem schem, utSym netName, bool isBus);
schConn schConnCreate(schWire wire1, schWire wire2, int32 x, int32 y);
schRect schRectCreate(schGraphic graphic, utBox box, uint8 color);
schSignal schSignalCreate(schNet net, schBus bus, utSym sym);
schBus schBusCreate(schSchem schem, utSym name, uint32 left, uint32 right);

/* Short-cuts and misc. routines */
char *schCompGetUserName(schComp comp);
utSym schFindAttrValue(schAttr attr, utSym name);
utSym schSchemCreateUniqueNetName(schSchem schem, char *name);
utSym schSchemCreateUniqueCompName(schSchem schem, char *name);
void schNetRename(schNet net, utSym newName);
utBox schWireFindBox(schWire wire);
dbMportType schFindFlagType(schComp comp);
bool schPinTypeOnRight(dbMportType type);
uint32 schFindTextSpace(utSym name);
void schSymbolInsertAttr(schSymbol symbol, schAttr attr);
void schMpinInsertAttr(schMpin mpin, schAttr attr);
bool schNameHasRange(char *name, uint32 *left, uint32 *right);
dbNetlistType schSymbolFindType(schSymbol symbol);
bool schPinIsBus(schPin pin);
#define schConnFindOtherWire(conn, wire) (schConnGetLWire(conn) == (wire)? schConnGetRWire(conn) : \
    schConnGetLWire(conn))

/* Wire --> conn iterator */
#define schWireGetFirstConn(wire) (schWireGetFirstLConn(wire) != schConnNull? \
    schWireGetFirstLConn(wire) : schWireGetFirstRConn(wire))
#define schConnGetNextWireConn(wire, conn) (schConnGetLWire(conn) == (wire)? \
    (schConnGetNextWireLConn(conn) != schConnNull? \
    schConnGetNextWireLConn(conn) : schWireGetFirstRConn(wire)) : \
    schConnGetNextWireRConn(conn))
#define schForeachWireConn(wire, conn) \
    for(conn = schWireGetFirstConn(wire); (conn) != schConnNull; \
    conn = schConnGetNextWireConn(wire, conn))
#define schEndWireConn

/* for text alignment */
/*   2 -- 5 -- 8  */
/*   |    |    |  */
/*   1 -- 4 -- 7  */
/*   |    |    |  */
/*   0 -- 3 -- 6  */
#define LOWER_LEFT      0
#define MIDDLE_LEFT     1
#define UPPER_LEFT      2
#define LOWER_MIDDLE    3
#define MIDDLE_MIDDLE   4
#define UPPER_MIDDLE    5
#define LOWER_RIGHT     6
#define MIDDLE_RIGHT    7
#define UPPER_RIGHT     8

#define SCH_CHAR_WIDTH 100
#define SCH_PIN_LENGTH 300
#define SCH_PIN_SPACE 400
#define SCH_PINLABEL_INSET 100

/* The root object */
extern schRoot schTheRoot;

/* utSym values */
extern utSym schRefdesSym, schPinlabelSym, schPinnumberSym, schPinseqSym, schPintypeSym;
extern utSym schSourceSym, schNetnameSym, schDeviceSym, schFlagSym;
extern utSym schPowerSym, schNetSym, schValueSym, schCommentSym;
