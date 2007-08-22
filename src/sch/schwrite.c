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
  Write a schematic.
--------------------------------------------------------------------------------------------------*/
#include "sch.h"

FILE *schFile;

/*--------------------------------------------------------------------------------------------------
  Write to schFile.
--------------------------------------------------------------------------------------------------*/
void schPrint(
    char *format,
    ...)
{
    va_list ap;

    va_start(ap, format);
    vfprintf(schFile, (char *)format, ap);
    va_end(ap);
}

/*--------------------------------------------------------------------------------------------------
  Write out an attribute;
--------------------------------------------------------------------------------------------------*/
static void writeAttrs(
    schAttr attr)
{
    utSym name, value;

    while(attr != schAttrNull) {
        schPrint("T %d %d %d %d %d %d 0 %d\n", schAttrGetX(attr), schAttrGetY(attr),
            schAttrGetColor(attr), schAttrGetSize(attr), schAttrVisible(attr)? 1 : 0,
            (schAttrShowName(attr)? 2 : 0) | (schAttrShowValue(attr)? 1 : 0),
            schAttrGetAlignment(attr));
        name = schAttrGetName(attr);
        value = schAttrGetValue(attr);
        if(value == utSymNull) {
            schPrint("%s\n", utSymGetName(name));
        } else {
            schPrint("%s=%s\n", utSymGetName(name), utSymGetName(value));
        }
        attr = schAttrGetNextAttr(attr);
    }
}

/*--------------------------------------------------------------------------------------------------
  Write out the mpins.
--------------------------------------------------------------------------------------------------*/
static void writeMpins(
    schSymbol symbol)
{
    schMpin mpin;
    schAttr attr;

    schForeachSymbolMpin(symbol, mpin) {
        schPrint("P %d %d %d %d 1 0 1\n", schMpinGetX2(mpin), schMpinGetY2(mpin),
            schMpinGetX(mpin), schMpinGetY(mpin));
        attr = schMpinGetAttr(mpin);
        if(attr != schAttrNull) {
            schPrint("{\n");
            writeAttrs(attr);
            schPrint("}\n");
        }
    } schEndSymbolMpin;
}

/*--------------------------------------------------------------------------------------------------
  Write a line to the file.
--------------------------------------------------------------------------------------------------*/
static void writeLine(
    schLine line)
{
    schPrint("L %d %d %d %d %d 0 0 0 -1 -1\n", schLineGetX1(line), schLineGetY1(line),
        schLineGetX2(line), schLineGetY2(line), schLineGetColor(line));
}

/*--------------------------------------------------------------------------------------------------
  Write a box to the file.
--------------------------------------------------------------------------------------------------*/
static void writeRect(
    schRect rect)
{
    utBox box = schRectGetBox(rect);

    schPrint("B %d %d %d %d %d 0 0 0 -1 -1 0 -1 -1 -1 -1 -1\n", utBoxGetLeft(box),
        utBoxGetBottom(box), utBoxGetWidth(box), utBoxGetHeight(box), schRectGetColor(rect));
}

/*--------------------------------------------------------------------------------------------------
  Write a circle to the file.
--------------------------------------------------------------------------------------------------*/
static void writeCircle(
    schCircle circle)
{
    schPrint("V %d %d %d %d 0 0 0 -1 -1 0 0 -1 -1 -1 -1\n",
        schCircleGetX(circle), schCircleGetY(circle), schCircleGetRadius(circle),
        schCircleGetColor(circle)); 
}

/*--------------------------------------------------------------------------------------------------
  Write an arc to the file.
--------------------------------------------------------------------------------------------------*/
static void writeArc(
    schArc arc)
{
    schPrint("A %d %d %d %d %d %d 0 0 0 -1 -1\n",
        schArcGetX(arc), schArcGetY(arc), schArcGetRadius(arc), schArcGetStartAngle(arc),
        schArcGetEndAngle(arc), schArcGetColor(arc));
}

/*--------------------------------------------------------------------------------------------------
  Write a graphic to the file.
--------------------------------------------------------------------------------------------------*/
static void writeGraphic(
    schGraphic graphic)
{
    schLine line;
    schRect rect;
    schCircle circle;
    schArc arc;

    schForeachGraphicLine(graphic, line) {
        writeLine(line);
    } schEndGraphicLine;
    schForeachGraphicRect(graphic, rect) {
        writeRect(rect);
    } schEndGraphicRect;
    schForeachGraphicCircle(graphic, circle) {
        writeCircle(circle);
    } schEndGraphicCircle;
    schForeachGraphicArc(graphic, arc) {
        writeArc(arc);
    } schEndGraphicArc;
}

/*--------------------------------------------------------------------------------------------------
  Write a symbol to the file.
--------------------------------------------------------------------------------------------------*/
static void writeSymbol(
    schSymbol symbol)
{
    schPrint("v 20030901\n");
    writeMpins(symbol);
    writeGraphic(schSymbolGetGraphic(symbol));
    writeAttrs(schSymbolGetAttr(symbol));
}

/*--------------------------------------------------------------------------------------------------
  Write a schematic.
--------------------------------------------------------------------------------------------------*/
bool schWriteSymbol(
    char *fileName,
    schSymbol symbol)
{
    schFile = fopen(fileName, "w");
    if(schFile == NULL) {
        utWarning("Could not write to file %s", fileName);
        return false;
    }
    writeSymbol(symbol);
    fclose(schFile);
    return true;
}

