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
  Generate a simple box symbol for the schematic.
--------------------------------------------------------------------------------------------------*/
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include "sch.h"

static schMpinArray schSymbolMpins;

/*--------------------------------------------------------------------------------------------------
  Allocate memory used in the symbol generator.
--------------------------------------------------------------------------------------------------*/
void schGensymStart(void)
{
    schSymbolMpins = schMpinArrayAlloc();
}

/*--------------------------------------------------------------------------------------------------
  Free memory used in the symbol generator.
--------------------------------------------------------------------------------------------------*/
void schGensymStop(void)
{
    schMpinArrayFree(schSymbolMpins);
}

/*--------------------------------------------------------------------------------------------------
  Find the name of the symbol for the schematic.
--------------------------------------------------------------------------------------------------*/
static utSym findSymbolName(
    schSchem schem)
{
    char *name = utReplaceSuffix(schSchemGetName(schem), "");
    char *version = strrchr(name, '_');
    char *p;

    if(version != NULL) {
        p = version;
        do {
            ++p;
        } while(isdigit(*p));
        if(*p == '\0') {
            *version = '-';
        }
    }
    return utSymCreate(utReplaceSuffix(name, ".sym"));
}

/*--------------------------------------------------------------------------------------------------
  Create a default graphic for the symbol.
--------------------------------------------------------------------------------------------------*/
static uint32 countSymbolInMpins(
    schSymbol symbol)
{
    schMpin mpin;
    uint32 numInMpins = 0;

    schForeachSymbolMpin(symbol, mpin) {
        if(!schPinTypeOnRight(schMpinGetType(mpin))) {
            numInMpins++;
        }
    } schEndSymbolMpin;
    return numInMpins;
}

/*--------------------------------------------------------------------------------------------------
  Create a default graphic for the symbol.
--------------------------------------------------------------------------------------------------*/
static uint32 countSymbolOutMpins(
    schSymbol symbol)
{
    schMpin mpin;
    uint32 numOutMpins = 0;

    schForeachSymbolMpin(symbol, mpin) {
        if(schPinTypeOnRight(schMpinGetType(mpin))) {
            numOutMpins++;
        }
    } schEndSymbolMpin;
    return numOutMpins;
}

/*--------------------------------------------------------------------------------------------------
  Estimate the space needed for the labels.
--------------------------------------------------------------------------------------------------*/
static uint32 findMpinLabelSpace(
    schSymbol symbol)
{
    schMpin mpin;
    uint32 space, maxLeftSpace = 0, maxRightSpace = 0;

    schForeachSymbolMpin(symbol, mpin) {
        space = schFindTextSpace(schMpinGetSym(mpin));
        if(schPinTypeOnRight(schMpinGetType(mpin))) {
            maxRightSpace = utMax(maxRightSpace, space);
        } else {
            maxLeftSpace = utMax(maxLeftSpace, space);
        }
    } schEndSymbolMpin;
    return 2*SCH_PIN_SPACE + maxLeftSpace + maxRightSpace;
}

/*--------------------------------------------------------------------------------------------------
  Find a suitable box for the symbol.
--------------------------------------------------------------------------------------------------*/
static utBox findSymbolBox(
    schSymbol symbol,
    uint32 numInPins,
    uint32 numOutPins)
{
    uint32 labelSpace = findMpinLabelSpace(symbol);

    return utMakeBox(SCH_PIN_LENGTH, SCH_PIN_LENGTH, SCH_PIN_LENGTH + labelSpace,
        SCH_PIN_LENGTH + (utMax(numInPins, numOutPins) + 1)*SCH_PIN_SPACE);
}

/*--------------------------------------------------------------------------------------------------
  Compare the Y coordinates of the mpins.
--------------------------------------------------------------------------------------------------*/
static int compareMpinYs(
    const void *mpin1Ptr,
    const void *mpin2Ptr)
{
    schMpin mpin1 = *(schMpin *)mpin1Ptr;
    schMpin mpin2 = *(schMpin *)mpin2Ptr;
    schComp comp1 = schMpinGetFlagComp(mpin1);
    schComp comp2 = schMpinGetFlagComp(mpin2);

    return schCompGetY(comp1) - schCompGetY(comp2);
}

/*--------------------------------------------------------------------------------------------------
  Set the pin positions on the symbol.
--------------------------------------------------------------------------------------------------*/
static void setPinPositions(
    schSymbol symbol,
    utBox box,
    uint32 numInMpins,
    uint32 numOutMpins)
{
    schMpin mpin;
    uint32 height = utBoxGetHeight(box);
    int32 x = 0;
    int32 y = SCH_PIN_LENGTH + (height - (numInMpins - 1)*SCH_PIN_SPACE)/2;

    schMpinArraySetUsedMpin(schSymbolMpins, 0);
    schSafeForeachSymbolMpin(symbol, mpin) {
        schMpinArrayAppendMpin(schSymbolMpins, mpin);
        schSymbolRemoveMpin(symbol, mpin);
    } schEndSafeSymbolMpin;
    qsort(schMpinArrayGetMpins(schSymbolMpins), schMpinArrayGetUsedMpin(schSymbolMpins),
        sizeof(schMpin), compareMpinYs);
    schForeachMpinArrayMpin(schSymbolMpins, mpin) {
        if(!schPinTypeOnRight(schMpinGetType(mpin))) {
            schSymbolInsertMpin(symbol, mpin);
            schMpinSetX(mpin, x);
            schMpinSetY(mpin, y);
            schMpinSetX2(mpin, x + SCH_PIN_LENGTH);
            schMpinSetY2(mpin, y);
            y += SCH_PIN_SPACE;
        }
    } schEndMpinArrayMpin;
    x = 2*SCH_PIN_LENGTH + utBoxGetWidth(box);
    y = SCH_PIN_LENGTH + (height - (numOutMpins - 1)*SCH_PIN_SPACE)/2;
    schForeachMpinArrayMpin(schSymbolMpins, mpin) {
        if(schPinTypeOnRight(schMpinGetType(mpin))) {
            schSymbolInsertMpin(symbol, mpin);
            schMpinSetX(mpin, x);
            schMpinSetY(mpin, y);
            schMpinSetX2(mpin, x - SCH_PIN_LENGTH);
            schMpinSetY2(mpin, y);
            y += SCH_PIN_SPACE;
        }
    } schEndMpinArrayMpin;
}

/*--------------------------------------------------------------------------------------------------
  Create a default graphic for the symbol.
--------------------------------------------------------------------------------------------------*/
static void createDefaultSymbolGraphic(
    schSymbol symbol)
{
    schGraphic graphic = schGraphicAlloc();
    schAttr refdesAttr, nameAttr;
    uint32 numInPins = countSymbolInMpins(symbol);
    uint32 numOutPins = countSymbolOutMpins(symbol);
    utBox box = findSymbolBox(symbol, numInPins, numOutPins);
    utSym symbolName;

    utAssert(schSymbolGetGraphic(symbol) == schGraphicNull);
    schSymbolSetGraphic(symbol, graphic);
    setPinPositions(symbol, box, numInPins, numOutPins);
    schRectCreate(graphic, box, GRAPHIC_COLOR);
    refdesAttr = schAttrCreate(utBoxGetLeft(box) + (utBoxGetWidth(box) >> 1), 0,
        schRefdesSym, utSymCreate("U?"), true, false, true,
        DETACHED_ATTRIBUTE_COLOR, 10, 0, LOWER_MIDDLE);
    schSymbolInsertAttr(symbol, refdesAttr);
    symbolName = utSymCreate(utReplaceSuffix(schSymbolGetName(symbol), ""));
    nameAttr = schAttrCreate(utBoxGetLeft(box) + (utBoxGetWidth(box) >> 1),
        SCH_PIN_LENGTH + SCH_PINLABEL_INSET, schCommentSym, symbolName, true, false, true,
        DETACHED_ATTRIBUTE_COLOR, 10, 0, LOWER_MIDDLE);
    schSymbolInsertAttr(symbol, nameAttr);
}

/*--------------------------------------------------------------------------------------------------
  Find the location to place a visible pin label relative to the pin.
--------------------------------------------------------------------------------------------------*/
static void findMpinLabelXY(
    schMpin mpin,
    int32 *x,
    int32 *y)
{
    if(schPinTypeOnRight(schMpinGetType(mpin))) {
        *x = schMpinGetX(mpin) - SCH_PIN_LENGTH - SCH_PINLABEL_INSET ;
    } else {
        *x = schMpinGetX(mpin) + SCH_PIN_LENGTH + SCH_PINLABEL_INSET ;
    }
    *y = schMpinGetY(mpin);
}

/*--------------------------------------------------------------------------------------------------
  Create a default symbol for the schematic.
--------------------------------------------------------------------------------------------------*/
static void createMpinAttrs(
    schComp flag,
    schMpin mpin)
{
    int32 x, y;
    schAttr pinLabel, pinNumber, pinSequence, pinType;
    bool onRight = schPinTypeOnRight(schMpinGetType(mpin));
    uint8 alignment = onRight? MIDDLE_RIGHT : MIDDLE_LEFT;

    findMpinLabelXY(mpin, &x, &y);
    pinLabel = schAttrCreate(x, y, schPinlabelSym, schMpinGetSym(mpin), true, false, true,
        ATTRIBUTE_COLOR, 8, 0, alignment);
    pinNumber = schAttrCreate(x, y, schPinnumberSym, schMpinGetSym(mpin), false, false, false,
        ATTRIBUTE_COLOR, 8, 0, alignment);
    pinSequence = schAttrCreate(x, y, schPinseqSym,
        utSymCreate(utSprintf("%d", schMpinGetSequence(mpin))), false, false, false,
        ATTRIBUTE_COLOR, 8, 0, alignment);
    pinType = schAttrCreate(x, y, schPintypeSym, dbFindPinTypeSym(schMpinGetType(mpin)),
        false, false, false, ATTRIBUTE_COLOR, 8, 0, alignment);
    schMpinInsertAttr(mpin, pinType);
    schMpinInsertAttr(mpin, pinSequence);
    schMpinInsertAttr(mpin, pinNumber);
    schMpinInsertAttr(mpin, pinLabel);
}

/*--------------------------------------------------------------------------------------------------
  Create a default symbol for the schematic.
--------------------------------------------------------------------------------------------------*/
void schSchemCreateDefaultSymbol(
    schSchem schem)
{
    utSym name = findSymbolName(schem);
    schSymbol symbol = schSymbolCreate(name, DB_SUBCIRCUIT, schSchemGetPath(schem));
    schComp comp;
    schMpin mpin;
    schAttr sourceAttr;
    dbMportType type;
    utSym device, mpinName;
    uint32 sequence = 1;
    bool isBus;
    uint32 left, right;

    schForeachSchemComp(schem, comp) {
        device = schSymbolGetDevice(schCompGetSymbol(comp));
        if(device == schFlagSym) {
            type = schFindFlagType(comp);
            mpinName = schFindAttrValue(schCompGetAttr(comp), schValueSym);
            if(mpinName == utSymNull) {
                utWarning("Unnamed flag found in schematic %s", schSchemGetName(schem));
            } else {
                isBus = dbNameHasRange(utSymGetName(mpinName), &left, &right);
                mpin = schMpinCreate(symbol, mpinName, comp, type, sequence, 0, 0, isBus, left, right);
                schMpinSetFlagComp(mpin, comp);
                schCompSetMpin(comp, mpin);
                schCompSetArray(comp, isBus);
                schCompSetLeft(comp, left);
                schCompSetRight(comp, right);
                sequence++;
            }
        }
    } schEndSchemComp;
    schSchemSetSymbol(schem, symbol);
    schSymbolSetSchem(symbol, schem);
    sourceAttr = schAttrCreate(0, 0, schSourceSym, schSchemGetSym(schem), false, false, false,
        DETACHED_ATTRIBUTE_COLOR, 8, 0, LOWER_LEFT);
    schSymbolInsertAttr(symbol, sourceAttr);
    createDefaultSymbolGraphic(symbol);
    schForeachSymbolMpin(symbol, mpin) {
        createMpinAttrs(comp, mpin);
    } schEndSymbolMpin;
}

/*--------------------------------------------------------------------------------------------------
  Generate a simple box symbol for the schematic.
--------------------------------------------------------------------------------------------------*/
bool schGenerateSymbolFile(
    char *schemFileName)
{
    schSchem schem;
    schSymbol symbol;
    bool passed;

    schStart();
    schem = schReadSchem(schemFileName, false);
    if(schem == schSchemNull) {
        schStop();
        return false;
    }
    schSchemCreateDefaultSymbol(schem);
    symbol = schSchemGetSymbol(schem);
    passed = schWriteSymbol(schSymbolGetName(symbol), symbol);
    schStop();
    return passed;
}
