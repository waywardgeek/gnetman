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

%{

#include "stdlib.h"
#include "cir.h"

static cirDevice cirCurrentDevice;

/*--------------------------------------------------------------------------------------------------
  Build a new attribute.
--------------------------------------------------------------------------------------------------*/
cirAttr cirAttrCreate(
    cirDevice device,
    utSym name,
    bool nameVisible,
    bool valueVisible,
    bool optional)
{
    cirAttr attr = cirAttrAlloc();

    cirAttrSetSym(attr, utSymGetLowerSym(name));
    cirAttrSetNameVisible(attr, nameVisible);
    cirAttrSetValueVisible(attr, valueVisible);
    cirAttrSetOptional(attr, optional);
    cirDeviceAppendAttr(device, attr);
    return attr;
}

%}

%union {
    utSym stringVal;
    utSym identVal;
    cirAttr attrVal;
    bool boolVal;
};

%token <stringVal> STRING
%token <identVal> IDENT

%%

goal: devices
;

devices: /* Empty */
| devices device
| devices '\n'
;

device: deviceHeader stuff '\n'
;

deviceHeader: STRING IDENT
{
    utSym name = utSymGetLowerSym($2);
    cirDevice oldDevice = cirRootFindDevice(cirTheRoot, name);
    if(oldDevice != cirDeviceNull) {
        /* Later definitions relace earlier ones */
        cirDeviceDestroy(oldDevice);
    }
    cirCurrentDevice = cirDeviceAlloc();
    cirDeviceSetSym(cirCurrentDevice, name);
    cirDeviceSetDescription(cirCurrentDevice, $1);
    cirRootInsertDevice(cirTheRoot, cirCurrentDevice);
}
;

stuff: /* Empty */
| stuff pin
| stuff attribute
;

pin: IDENT
{
    cirPin pin = cirPinAlloc();
    cirPinSetSym(pin, $1);
    cirDeviceAppendPin(cirCurrentDevice, pin);
}
;

attribute: '<' IDENT '>'
{
    cirAttrCreate(cirCurrentDevice, $2, false, true, false);
}
| IDENT '=' '<' IDENT '>'
{
    cirAttrCreate(cirCurrentDevice, $1, true, true, false);
}
| '[' IDENT ']'
{
    cirAttrCreate(cirCurrentDevice, $2, false, true, true);
}
| '[' IDENT '=' '<' IDENT '>' ']'
{
    cirAttrCreate(cirCurrentDevice, $2, true, true, true);
}
| '[' '<' IDENT '>' ']'
{
    cirAttrCreate(cirCurrentDevice, $3, false, true, true);
}
;

%%
