/*
 * Copyright (C) 2004 ViASIC
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
#include "vr.h"

static dbNetlist vrCurrentNetlist;
static bool vrIsBus;
static uint32 vrLeftRange, vrRightRange;
static dbDirection vrMportType;
static vrIdec vrCurrentIdec;
static vrParam vrCurrentParam;
static vrDefparam vrCurrentDefparam;

/*--------------------------------------------------------------------------------------------------
  Build a simple assignment to a single net.
--------------------------------------------------------------------------------------------------*/
static void buildJoin(
    dbNet dest,
    dbNet source)
{
    if(source == dest) {
        vrerror("assignment of signal to itself on signal %s", dbNetGetName(source));
    }
    dbJoinNets(source, dest);
}

/*--------------------------------------------------------------------------------------------------
  Just look-up the net, and give an error if not found.
--------------------------------------------------------------------------------------------------*/
static dbNet findNet(
    utSym sym)
{
    dbNet net = dbNetlistFindNet(vrCurrentNetlist, sym);

    if(net == dbNetNull) {
        vrerror("Net %s not defined", utSymGetName(sym));
    }
    return net;
}

/*--------------------------------------------------------------------------------------------------
  Build a simple assignment from a net to a net.
--------------------------------------------------------------------------------------------------*/
static dbNet findConstantNet(
    uint32 value)
{
    if(value == 0) {
        return dbNetlistGetZeroNet(vrCurrentNetlist);
    } else if(value == 1) {
        return dbNetlistGetOneNet(vrCurrentNetlist);
    } else {
        vrerror("Only 0 and 1 integer values currently supported");
    }
    return dbNetNull; /* Dummy return */
}

/*--------------------------------------------------------------------------------------------------
  Declare a signal
--------------------------------------------------------------------------------------------------*/
static void declareSignal(
    dbNetlist netlist,
    utSym sym,
    bool isBus,
    uint32 left,
    uint32 right,
    dbMportType type)
{
    dbMport mport = dbNetlistFindMport(netlist, sym);
    dbBus bus;
    dbMbus mbus;
    dbNet net;
    dbInst flag;

    if(!isBus) {
        if(type != DB_NODIR || dbNetlistFindNet(netlist, sym) == dbNetNull) {
            net = dbNetCreate(netlist, sym);
            if(mport != dbMportNull) {
                dbMportSetType(mport, type);
                flag = dbFlagInstCreate(mport);
                dbNetAppendPort(net, dbInstGetFirstPort(flag));
            }
            if(mport == dbMportNull && type != DB_NODIR) {
                vrerror("Net %s not declared in I/O list for module %s",
                        utSymGetName(sym), dbNetlistGetName(netlist));
            } else if(mport != dbMportNull && type == DB_NODIR) {
                vrerror("I/O net %s declared as wire in module %s",
                        utSymGetName(sym), dbNetlistGetName(netlist));
            }
        }
    } else {
        if(type != DB_NODIR || dbNetlistFindBus(netlist, sym) == dbBusNull) {
            bus = dbBusCreate(netlist, sym, left, right);
            if(mport != dbMportNull) {
                mbus = dbMbusCreate(netlist, sym, type, left, right);
                dbMportDestroy(mport);
                flag = dbBusFlagInstCreate(mbus);
                dbBusHookup(bus, dbInstGetFirstPort(flag));
            }
        }
    }
}

/*--------------------------------------------------------------------------------------------------
  Build a connection.
--------------------------------------------------------------------------------------------------*/
static vrConn vrConnCreate(
    vrParam oParam,
    utSym sym, 
    bool hasRange,
    uint32 left,
    uint32 right,
    bool isConst,
    uint32 length,
    uint32 mask)
{
    dbBus bus;
    dbNet net;
    vrConn conn = vrConnAlloc();

    if(sym != utSymNull) {
        bus = dbNetlistFindBus(vrCurrentNetlist, sym);
        vrConnSetBus(conn, bus);
        if(bus == dbBusNull) {
            if(hasRange) {
                vrerror("Indexed scalar signal %s", utSymGetName(sym));
            }
            net = dbNetlistFindNet(vrCurrentNetlist, sym);
            if(net == dbNetNull) {
                declareSignal(vrCurrentNetlist, sym, false, 0, 0, DB_NODIR);
                net = dbNetlistFindNet(vrCurrentNetlist, sym);
                utAssert(net != dbNetNull);
            }
            vrConnSetNet(conn, net);
        }
    } else {
        utAssert(isConst);
    }
    vrParamAppendConn(oParam, conn);
    vrConnSetRange(conn, hasRange);
    vrConnSetLeft(conn, left);
    vrConnSetRight(conn, right);
    vrConnSetConst(conn, isConst);
    vrConnSetLength(conn, length);
    vrConnSetMask(conn, mask);
    return conn;
}

/*--------------------------------------------------------------------------------------------------
  Provide yyerror function capability.
--------------------------------------------------------------------------------------------------*/
void vrerror(
    char *message,
    ...)
{
    char *buff;
    va_list ap;

    va_start(ap, message);
    buff = utVsprintf(message, ap);
    va_end(ap);
    utError("Line %d, token \"%s\": %s", vrLineNum, vrtext, buff);
}

%}

%union {
    utSym symVal, stringVal;
    uint32 intVal;
    double doubleVal;
    struct {
        uint32 mask;
        uint8 length;
    } constVal;
    dbNet netVal;
};

%token <symVal> IDENT
%token <intVal> INTEGER
%token <doubleVal> DOUBLE
%token <constVal> CONSTANT
%token <stringVal> STRING

%type <netVal> signal lSignal
%type <symVal> value

%token KWMODULE KWENDMODULE KWINPUT KWOUTPUT KWWIRE KWINOUT KWASSIGN
%token KWPARAMETER KWDEFPARAM

%%

goal: modules
{ dbDesignSetRootNetlist(vrCurrentDesign, vrCurrentNetlist); }
;

modules: /* empty */
| modules module
;

module: moduleHeader '(' ioList ')' ';' declarations statements KWENDMODULE
;

moduleHeader: KWMODULE IDENT
{ vrCurrentNetlist = vrNetlistCreate(vrCurrentDesign, $2); }
;

ioList: /* empty */
| ioList2
;

ioList2: IDENT
{ dbMportCreate(vrCurrentNetlist, $1, DB_IN); }
| ioList2 ',' IDENT
{ dbMportCreate(vrCurrentNetlist, $3, DB_IN); }
;

declarations: /* empty */
| declarations declaration
;

declaration: type signalList ';'
| KWPARAMETER parameters ';'
;

parameters: parameter
| parameters ',' parameter
;

parameter: IDENT
{ ; }
| IDENT '=' value
{ dbNetlistSetValue(vrCurrentNetlist, $1, $3); }
;

value: IDENT
{ $$ = $1; }
| INTEGER
{ $$ = utSymCreateFormatted("%d", $1); }
| DOUBLE
{ $$ = utSymCreateFormatted("%f", $1); }
| STRING
{ $$ = $1; }
;

type: KWINPUT optRange
{ vrMportType = DB_IN; }
| KWOUTPUT optRange
{ vrMportType = DB_OUT; }
| KWINOUT optRange
{ vrMportType = DB_IO; }
| KWWIRE optRange
{ vrMportType = DB_NODIR; }
;

optRange: /* empty */
{ vrIsBus = false; }
| range
{ vrIsBus = true; }
;

range : '[' INTEGER ':' INTEGER ']'
{ vrLeftRange = $2;
 vrRightRange = $4; }
;

signalList: signalDef
| signalList ',' signalDef
;

signalDef: IDENT
{ declareSignal(vrCurrentNetlist, $1, vrIsBus, vrLeftRange, vrRightRange, vrMportType); }
;

statements: /* empty */
| statements statement
;

statement: instance
| assignment
| defparamStatement
;

defparamStatement: KWDEFPARAM defparams ';'
;

defparams: defparam
| defparams ',' defparam
;

defparam: startParam paramPath '.' IDENT '=' value
{ vrDefparamSetSym(vrCurrentDefparam, $4);
  vrDefparamSetValue(vrCurrentDefparam, $6); }
;

startParam: /* Empty */
{ vrCurrentDefparam = vrDefparamAlloc();
  vrNetlistInsertDefparam(vrCurrentNetlist, vrCurrentDefparam); }
;

paramPath: IDENT
{ vrPath path = vrPathAlloc();
  vrPathSetSym(path, $1);
  vrDefparamAppendPath(vrCurrentDefparam, path); }
| paramPath '.' IDENT
{ vrPath path = vrPathAlloc();
  vrPathSetSym(path, $3);
  vrDefparamAppendPath(vrCurrentDefparam, path); }
;

instance: instanceHeader '(' instParams ')' ';'
;

instanceHeader: IDENT IDENT
{ vrCurrentIdec = vrIdecAlloc();
  vrNetlistAppendIdec(vrCurrentNetlist, vrCurrentIdec);
  vrIdecSetInternalNetlistSym(vrCurrentIdec, $1);
  vrIdecSetSym(vrCurrentIdec, $2);
  vrIdecSetLineNum(vrCurrentIdec, vrLineNum); }
;

instParams: /* empty */
| instParams2
;

instParams2: instParam
| instParams ',' instParam
;

instParam: instParamHeader '(' paramConnection ')'
;

instParamHeader: '.' IDENT
{ vrCurrentParam = vrParamAlloc();
  vrParamSetSym(vrCurrentParam, $2);
  vrIdecAppendParam(vrCurrentIdec, vrCurrentParam); }
;

paramConnection: /* Empty */
| connection
| '{' connectionList '}'
;

connectionList: connection
| connectionList ',' connection
;

connection: IDENT
{ vrConnCreate(vrCurrentParam, $1, false, 0, 0, false, 0, 0); }
| IDENT range
{ vrConnCreate(vrCurrentParam, $1, true, vrLeftRange, vrRightRange, false, 0, 0); }
| IDENT '[' INTEGER ']'
{ vrConnCreate(vrCurrentParam, $1, true, $3, $3, false, 0, 0); }
| CONSTANT
{ vrConnCreate(vrCurrentParam, utSymNull, false, 0, 0, true, $1.length, $1.mask); }
;

assignment: KWASSIGN lSignal '=' signal ';'
{ buildJoin($2, $4); }
;

signal: lSignal
| INTEGER /* For VCC and GND */
{ $$ = findConstantNet($1); }
| CONSTANT /* For VCC and GND */
{ $$ = findConstantNet($1.mask); }
;

lSignal: IDENT
{ $$ = findNet($1); }
| IDENT '[' INTEGER ']'
{ $$ = findNet(utSymCreateFormatted("%s[%d]", utSymGetName($1), $3)); }
;

%%
