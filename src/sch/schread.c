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
  Read a schematic.
--------------------------------------------------------------------------------------------------*/
#include <stdlib.h>
#include <string.h>
#include "sch.h"

FILE *schFile;
uint32 schLineNum;
char *schFileName;
static char *schLineBuf;
static uint32 schLineLength, schLineSize;

/*--------------------------------------------------------------------------------------------------
  Initialize the read module
--------------------------------------------------------------------------------------------------*/
void schStartReader(void)
{
    schLineLength = 0;
    schLineSize = 42;
    schLineBuf = utNewA(char, schLineSize);
}

/*--------------------------------------------------------------------------------------------------
  Close the read module
--------------------------------------------------------------------------------------------------*/
void schStopReader(void)
{
    utFree(schLineBuf);
}

/*--------------------------------------------------------------------------------------------------
  Peek at the next character in the input stream.
--------------------------------------------------------------------------------------------------*/
static int peekChar(void)
{
    int c = getc(schFile);

    ungetc(c, schFile);
    return c;
}

/*--------------------------------------------------------------------------------------------------
  Read the next line.
--------------------------------------------------------------------------------------------------*/
static char *readLine(
    uint32 numLines)
{
    int c;

    schLineLength = 0;
    while(numLines-- != 0) {
        utDo {
            c = getc(schFile);
        } utWhile(c != EOF && c != '\n') {
            if(c != '\r') {
                if(schLineLength == schLineSize) {
                    schLineSize <<= 1;
                    utResizeArray(schLineBuf, schLineSize);
                }
                schLineBuf[schLineLength++] = c;
            }
        } utRepeat;
        if(c == '\n') {
            schLineNum++;
        } else if(schLineLength == 0) {
            return NULL; /* End of input */
        }
        if(schLineLength == schLineSize) {
            schLineSize <<= 1;
            utResizeArray(schLineBuf, schLineSize);
        }
        schLineBuf[schLineLength++] = '\n';
    }
    schLineBuf[schLineLength - 1] = '\0';
    return schLineBuf;
}

/*--------------------------------------------------------------------------------------------------
  Report a warning during reading the schematic file.
--------------------------------------------------------------------------------------------------*/
void schWarning(
    char *format,
    ...)
{
    va_list ap;
    char *buf;

    va_start(ap, format);
    buf = utVsprintf((char *)format, ap);
    va_end(ap);
    utWarning("File %s, line %u: %s\n", schFileName, schLineNum, buf);
}

/*--------------------------------------------------------------------------------------------------
  Read the attribute.  Convert text starting with ^ into spicetext attributes.
--------------------------------------------------------------------------------------------------*/
static schAttr readAttr(
    char *buf)
{
    char type;
    int32 x, y;
    int32 color;
    int32 size;
    int32 visibility;
    int32 showNameValue;
    int32 angle;
    int32 alignment;
    uint32 numLines = 1; /* Need to set here in case it's not given on the line */
    char *value, *string, *name;

    sscanf(buf, "%c %d %d %d %d %d %d %d %d %d", &type, &x, &y, &color, &size,
           &visibility, &showNameValue, &angle, &alignment, &numLines);
    string = readLine(numLines);
    if(*string == '^') {
        value = string + 1;
        name = "spicetext";
    } else {
        value = strchr(string, '=');
        if(value == NULL) {
            return schAttrNull; /* It was just some comment text */
        }
        *value++ = '\0';
        name = string;
    }
    return schAttrCreate(x, y, utSymCreate(name), utSymCreate(value), visibility != 0,
        (showNameValue & 2) != 0, (showNameValue & 1) != 0,
        (uint8)color, (uint8)size, (uint8)angle, (uint8)alignment);
}

/*--------------------------------------------------------------------------------------------------
  Read attached attributes.
--------------------------------------------------------------------------------------------------*/
static schAttr readAttributes(void)
{
    schAttr attr, firstAttr = schAttrNull, lastAttr = schAttrNull;
    char *buf;
    char objType;

    if(peekChar() != STARTATTACH_ATTR) {
        return schAttrNull;
    }
    readLine(1);
    utDo {
        objType = peekChar();
    } utWhile(objType == OBJ_TEXT) {
        buf = utCopyString(readLine(1));
        attr = readAttr(buf);
        if(attr != schAttrNull) {
            if(firstAttr == schAttrNull) {
                firstAttr = attr;
                lastAttr = attr;
            } else {
                schAttrSetNextAttr(lastAttr, attr);
                lastAttr = attr;
            }
        }
    } utRepeat;
    readLine(1);
    return firstAttr;
}

/*--------------------------------------------------------------------------------------------------
  Create a wire from the string.
--------------------------------------------------------------------------------------------------*/
static schWire readWire(
    schSchem schem,
    char *buf)
{
    schWire wire;
    schAttr attr;
    char type;
    int32 x1, y1;
    int32 x2, y2;
    int32 color;

    sscanf(buf, "%c %d %d %d %d %d", &type, &x1, &y1, &x2, &y2, &color);
    wire = schWireCreate(schem, x1, y1, x2, y2, false);
    attr = readAttributes();
    schWireSetAttr(wire, attr);
    return wire;
}

/*--------------------------------------------------------------------------------------------------
  Create a bus wire from the string.
--------------------------------------------------------------------------------------------------*/
static schWire readBusWire(
    schSchem schem,
    char *buf)
{
    schWire wire;
    schAttr attr;
    char type;
    int32 x1, y1;
    int32 x2, y2;
    int32 color;
    int32 ripperDir;

    sscanf(buf, "%c %d %d %d %d %d %d", &type, &x1, &y1, &x2, &y2, &color, &ripperDir);
    wire = schWireCreate(schem, x1, y1, x2, y2, true);
    attr = readAttributes();
    schWireSetAttr(wire, attr);
    return wire;
}
 
/*--------------------------------------------------------------------------------------------------
  Read the component object.
--------------------------------------------------------------------------------------------------*/
static schComp readComp(
    schSchem schem,
    char *buf)
{
    schComp comp;
    schAttr attr;
    char type;
    int32 x, y;
    int32 angle;
    int32 selectable;
    int32 mirror;
    char symbolName[UTSTRLEN];
    char *name;
    utSym nameSym = utSymNull;

    sscanf(buf, "%c %d %d %d %d %d %s",
         &type, &x, &y, &selectable, &angle, &mirror, symbolName);
    if (strncmp(symbolName, "EMBEDDED", 8) == 0) {
        schWarning("Embedded components not yet supported");
    }
    attr = readAttributes();
    nameSym = schFindAttrValue(attr, schRefdesSym);
    if(nameSym != utSymNull) {
        name = utSymGetName(nameSym);
        if(name[strlen(name)] == '?') {
            schWarning("Unnamed %s component in schematic %s at (%d, %d)", symbolName,
                schSchemGetName(schem), x, y);
            nameSym = utSymNull;
        }
    }
    if(nameSym != utSymNull) {
        comp = schSchemFindComp(schem, nameSym);
        if(comp != schCompNull) {
            schWarning("Component %s in schematic %s at (%d, %d) already defined",
                utSymGetName(nameSym), schSchemGetName(schem), x, y);
            nameSym = utSymNull;
        }
    }
    comp = schCompCreate(schem, nameSym, utSymCreate(symbolName), x, y, selectable != 0,
        (uint16)angle, mirror != 0);
    schCompSetAttr(comp, attr);
    return comp;
}

/*--------------------------------------------------------------------------------------------------
  Read the mpin.
--------------------------------------------------------------------------------------------------*/
static schMpin readMpin(
    schSymbol symbol,
    char *buf)
{
    schMpin mpin;
    schAttr attr;
    char type;
    int32 x1, y1;
    int32 x2, y2;
    int32 x, y;
    int32 color;
    int32 pinType;
    int32 whichend;
    uint32 sequence = 0;
    utSym name, pinTypeSym;
    utSym pinseq;
    bool isBus;
    uint32 left, right;

    /* What is the 'pinType' field for? It doesn't seem to have to do with the pintype attribute */
    sscanf(buf, "%c %d %d %d %d %d %d %d", &type, &x1, &y1, &x2, &y2, &color, &pinType, &whichend);
    if(whichend == 0) {
        x = x1;
        y = y1;
    } else {
        x = x2;
        y = y2;
    }
    attr = readAttributes();
    name = schFindAttrValue(attr, schPinlabelSym);
    if(name == utSymNull) {
        schWarning("Pin is missing pinlabel attribute");
    }
    pinTypeSym = schFindAttrValue(attr, schPintypeSym);
    if(pinTypeSym == utSymNull) {
        pinTypeSym = dbFindPinTypeSym(DB_IN);
    }
    pinseq = schFindAttrValue(attr, schPinseqSym);
    if(pinseq == utSymNull) {
        schWarning("Pin is missing pinseq attribute");
    } else {
        sequence = atol(utSymGetName(pinseq));
    }
    isBus = dbNameHasRange(utSymGetName(name), &left, &right);
    mpin = schMpinCreate(symbol, name, schCompNull, dbFindPinTypeFromSym(pinTypeSym), sequence,
        x, y, isBus, left, right);
    schMpinSetAttr(mpin, attr);
    return mpin;
}

/*--------------------------------------------------------------------------------------------------
  Read the schematic.
--------------------------------------------------------------------------------------------------*/
static schSchem readSchem(
    utSym name,
    utSym path)
{
    schSchem schem = schSchemCreate(name, path);
    schAttr attr, lastAttr = schAttrNull;
    char *buf;
    char objType;

    utDo {
        buf = readLine(1);
    } utWhile(buf != NULL) {
        objType = buf[0];
        switch(objType) {
        case OBJ_LINE:
        case OBJ_ARC:
        case OBJ_BOX:
        case OBJ_CIRCLE:
        case INFO_FONT:
        case COMMENT:
        case ENDATTACH_ATTR:
            /* These don't seem to have netlist info */
            break;
        case OBJ_NET:
            readWire(schem, buf);
            break;
        case OBJ_BUS:
            readBusWire(schem, buf);
            break;
        case OBJ_COMPLEX:
            readComp(schem, buf);
            break;
        case OBJ_TEXT:
            buf = utCopyString(buf);
            attr = readAttr(buf);
            if(attr != schAttrNull) {
                if(lastAttr == schAttrNull) {
                    schSchemSetAttr(schem, attr);
                    lastAttr = attr;
                } else {
                    schAttrSetNextAttr(lastAttr, attr);
                    lastAttr = attr;
                }
            }
            break;
        case OBJ_PIN:
            schWarning("Unexpected pin declaration in schematic");
            break;
        case STARTATTACH_ATTR:
            schWarning("No object to attach attributes to");
            break;
        case START_EMBEDDED:
            schWarning("Embedded components not supported yet.");
            return false;
        case END_EMBEDDED:
            break;
        case VERSION_CHAR:
            break;
        default:
            schWarning("Unknown object type '%c'\n", objType);
            return false;
        }
    } utRepeat;
    return schem;
}

/*--------------------------------------------------------------------------------------------------
  Read the schematic.
--------------------------------------------------------------------------------------------------*/
static schSymbol readSymbol(
    utSym name,
    utSym path)
{
    schSymbol symbol = schSymbolCreate(name, DB_SUBCIRCUIT, path);
    schAttr attr, lastAttr = schAttrNull;
    utSym device;
    char *buf;
    char objType;

    utDo {
        buf = readLine(1);
    } utWhile(buf != NULL) {
        objType = buf[0];
        switch(objType) {
        case OBJ_LINE:
        case OBJ_ARC:
        case OBJ_BOX:
        case OBJ_CIRCLE:
        case INFO_FONT:
        case COMMENT:
        case ENDATTACH_ATTR:
            /* These don't seem to have netlist info */
            break;
        case OBJ_NET:
        case OBJ_COMPLEX:
            schWarning("Unexpected object in symbol");
            break;
        case OBJ_TEXT:
            buf = utCopyString(buf);
            attr = readAttr(buf);
            if(attr != schAttrNull) {
                if(lastAttr == schAttrNull) {
                    schSymbolSetAttr(symbol, attr);
                    lastAttr = attr;
                } else {
                    schAttrSetNextAttr(lastAttr, attr);
                    lastAttr = attr;
                }
            }
            break;
        case OBJ_PIN:
            readMpin(symbol, buf);
            break;
        case STARTATTACH_ATTR:
            schWarning("No object to attach attributes to");
            break;
        case START_EMBEDDED:
            schWarning("Embedded components not supported yet.");
            return false;
        case END_EMBEDDED:
            break;
        case VERSION_CHAR:
            break;
        default:
            schWarning("Unknown object type '%c'\n", objType);
            return false;
        }
    } utRepeat;
    device = schFindAttrValue(schSymbolGetAttr(symbol), schDeviceSym);
    schSymbolSetDevice(symbol, device);
    return symbol;
}

/*--------------------------------------------------------------------------------------------------
  Read the schematic.
--------------------------------------------------------------------------------------------------*/
schSchem schReadSchem(
    char *fileName,
    bool loadSubSchems)
{
    schSchem schem;
    utSym path;

    fileName = utFullPath(fileName);
    schFileName = fileName;
    schFile = fopen(fileName, "r");
    schLineNum = 1;
    if(schFile == NULL) {
        utWarning("Could not open file %s", fileName);
        return schSchemNull;
    }
    path = utSymCreate(utDirName(fileName));
    schem = readSchem(utSymCreate(utBaseName(fileName)), path);
    fclose(schFile);
    if(schem != schSchemNull) {
        if(!schSchemPostProcess(schem, loadSubSchems)) {
            schSchemDestroy(schem);
            return schSchemNull;
        }
    }
    return schem;
}

/*--------------------------------------------------------------------------------------------------
  Read the symbol.
--------------------------------------------------------------------------------------------------*/
schSymbol schReadSymbol(
    char *fileName,
    bool loadSubSchems)
{
    schSymbol symbol;
    utSym path;

    fileName = utFullPath(fileName);
    schFileName = fileName;
    schFile = fopen(fileName, "r");
    schLineNum = 1;
    if(schFile == NULL) {
        utWarning("Could not open file %s", fileName);
        return schSymbolNull;
    }
    path = utSymCreate(utDirName(fileName));
    symbol = readSymbol(utSymCreate(utBaseName(fileName)), path);
    fclose(schFile);
    if(symbol != schSymbolNull) {
        if(!schSymbolPostProcess(symbol, loadSubSchems)) {
            schSymbolDestroy(symbol);
            return schSymbolNull;
        }
    }
    return symbol;
}
