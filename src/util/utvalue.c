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

#include "ddutil.h"
#include "utvalue.h"

/*--------------------------------------------------------------------------------------------------
  Make an integer value.
--------------------------------------------------------------------------------------------------*/
utValue utMakeIntValue(
    int32 intValue)
{
    utValue value;

    value.type = UT_INTEGER_VALUE;
    value.u.intValue = intValue;
    return value;
}

/*--------------------------------------------------------------------------------------------------
  Make a sym value.
--------------------------------------------------------------------------------------------------*/
utValue utMakeSymValue(
    utSym sym)
{
    utValue value;

    value.type = UT_SYM_VALUE;
    value.u.sym = sym;
    return value;
}

/*--------------------------------------------------------------------------------------------------
  Make a string value.
--------------------------------------------------------------------------------------------------*/
utValue utMakeStringValue(
    char *string)
{
    utValue value;

    value.type = UT_STRING_VALUE;
    value.u.string = utSymCreate(string);
    return value;
}

/*--------------------------------------------------------------------------------------------------
  Make an boolean value.
--------------------------------------------------------------------------------------------------*/
utValue utMakeBooleanValue(
    bool boolValue)
{
    utValue value;

    value.type = UT_BOOLEAN_VALUE;
    value.u.boolValue = boolValue;
    return value;
}

/*--------------------------------------------------------------------------------------------------
  Make an double value.
--------------------------------------------------------------------------------------------------*/
utValue utMakeDoubleValue(
    double doubleValue)
{
    utValue value;

    value.type = UT_DOUBLE_VALUE;
    value.u.doubleValue = doubleValue;
    return value;
}

/*--------------------------------------------------------------------------------------------------
  Make a char value.
--------------------------------------------------------------------------------------------------*/
utValue utMakeCharValue(
    int c)
{
    utValue value;

    value.type = UT_CHAR_VALUE;
    value.u.c = c;
    return value;
}

/*--------------------------------------------------------------------------------------------------
  Make an null value.  This is for values that aren't used.
--------------------------------------------------------------------------------------------------*/
utValue utMakeNullValue(void)
{
    utValue value;

    value.type = UT_NULL_VALUE;
    value.u.intValue = 0;
    return value;
}

/*--------------------------------------------------------------------------------------------------
  Make a color value.
--------------------------------------------------------------------------------------------------*/
utColor utMakeColor(
    uint8 red,
    uint8 green,
    uint8 blue)
{
    utColor color;

    color.red = red;
    color.green = green;
    color.blue = blue;
    return color;
}
