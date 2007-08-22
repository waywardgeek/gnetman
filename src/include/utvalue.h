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

#define UTVALUE_H

typedef enum {
    UT_INTEGER_VALUE,
    UT_BOOLEAN_VALUE,
    UT_STRING_VALUE,
    UT_SYM_VALUE,
    UT_DOUBLE_VALUE,
    UT_CHAR_VALUE,
    UT_NULL_VALUE
} utValueType;

typedef struct {
    utValueType type;
    union {
        int32 intValue;
        utSym sym, string;
        bool boolValue;
        double doubleValue;
        int c;
    } u;
} utValue;

#define utgValueType(value) ((value).type)
#define utgValueInt(value) ((value).u.intValue)
#define utgValueSym(value) ((value).u.sym)
#define utgValueBoolean(value) ((value).u.boolValue)
#define utgValueDouble(value) ((value).u.doubleValue)
#define utgValueString(value) utgSymName((value).u.string)
#define utgValueStringSym(value) ((value).u.string)
#define utgValueChar(value) ((value).u.c)
#define uttValueNull(value) ((value).type == UT_NULL_VALUE)

extern utValue utMakeNullValue(void);
extern utValue utMakeIntValue(int32 intValue);
extern utValue utMakeSymValue(utSym sym);
extern utValue utMakeBooleanValue(bool boolValue);
extern utValue utMakeDoubleValue(double doubleValue);
extern utValue utMakeStringValue(char *string);
extern utValue utMakeCharValue(int c);

typedef struct {
    uint8 red, green, blue;
} utColor;

#define utgColorRed(color) ((color).red)
#define utgColorGreen(color) ((color).green)
#define utgColorBlue(color) ((color).blue)
extern utColor utMakeColor(uint8 red, uint8 green, uint8 blue);

