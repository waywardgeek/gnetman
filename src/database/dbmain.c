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
  Top level database functions.
--------------------------------------------------------------------------------------------------*/
#include <string.h>
#include "db.h"

dbRoot dbTheRoot;
utSym dbGraphicalSym;

/* This defines devices for the spice netlister to LTSpice */
char dbDefaultSpiceDeviceString[] =
"'arbitrary behavioral source' B n1 n2 [V=<equation>] [I=<equation>]\n"
"'capacitor' C n1 n2 <value> [ic=<value>] [Rser=<value>]"
"    [Lser=<value>] [Rpar=<value>] [Cpar=<value>] [m=<value>]\n"
"'diode' D A K <model> [<area>]\n"
"'voltage dependent voltage' E n1 n2 nc+ nc- <gain>\n"
"'current dependent current' F n1 n2 <Vnam> <gain>\n"
"'voltage dependent current' G n1 n2 nc+ nc- <transconductance>\n"
"'current dependent voltage' H n1 n2 <Vnam> <transres>\n"
"'independent current source' I n1 n2 <current>\n"
"'JFET transistor' J D G S <model> [<area>] [off] [IC=<Vds,Vgs>] [temp=<T>]\n"
"'mutual inductance' K L1 L2 <coefficient>\n"
"'inductance' L n1 n2 <inductance> [ic=<value>] [Rser=<value>] [Rpar=<value>]"
"    [Cpar=<value>] [m=<value>]\n"
"'MOSFET transistor' M D G S B <model> [L=<length>] [W=<width>] [AD=<area>] [AS=<area>]"
"    [PD=<perim>] [PS=<perim>] [NRD=<value>] [NRS=<value>] [off]"
"    [IC=<Vds,Vgs,Vbs>] [temp=<T>]\n"
"'lossy transmission line' O L+ L- R+ R- <model>\n"
"'bipolar transistor' Q C B E [S] <model> [<area>] [off] [IC=<Vbe,Vce>] [temp=<T>]\n"
"'resistance' R n1 n2 <value>\n"
"'voltage controlled switch' S n1 n2 nc+ nc- <model> [on] [off]\n"
"'lossless transmission line' T L+ L- R+ R- ZO=<value> TD=<value>\n"
"'uniform RC-line' U n1 n2 ncommon <model> L=<len> [N=<lumps>]\n"
"'independent voltage source' V n1 n2 <voltage>\n"
"'current controlled switch' W n1 n2 <Vnam> <model> [on] [off]\n"
"'MESFET transistor' Z D G S <model> [<area>] [off] [IC=<Vds,Vgs>]\n";

/*--------------------------------------------------------------------------------------------------
  Start up the database.
--------------------------------------------------------------------------------------------------*/
void dbStart(void)
{
    dbDatabaseStart();
    dbTheRoot = dbRootAlloc();
    dbRootAllocGschemComponentPaths(dbTheRoot, 2);
    strcpy(dbGschemComponentPath, ".");
    dbRootAllocGschemSourcePaths(dbTheRoot, 2);
    strcpy(dbGschemSourcePath, ".");
    dbRootSetSpiceTarget(dbTheRoot, DB_LTSPICE);
    dbRootSetCurrentDesign(dbTheRoot, dbDesignNull);
    dbRootSetCurrentLibrary(dbTheRoot, dbDesignNull);
    dbRootSetCurrentNetlist(dbTheRoot, dbNetlistNull);
    dbRootSetLibraryWins(dbTheRoot, false);
    /* Set a reasonable default SPICE string, in case there's no config file */
    dbDevspecCreate(utSymCreate("ltspice"), DB_LTSPICE, dbDefaultSpiceDeviceString);
    dbDevspecCreate(utSymCreate("tclspice"), DB_TCLSPICE, "");
    dbDevspecCreate(utSymCreate("hspice"), DB_HSPICE, "");
    dbDevspecCreate(utSymCreate("pspice"), DB_PSPICE, "");
    dbDevspecCreate(utSymCreate("cdl"), DB_CDL, "");
    dbRootSetDefaultOneSym(dbTheRoot, utSymCreate("VDD"));
    dbRootSetDefaultZeroSym(dbTheRoot, utSymCreate("VSS"));
    dbGraphicalSym = utSymCreate("graphical");
    /* Temp hack */
    geRES250Sym = utSymCreate("RES250");
    geRES6KSym = utSymCreate("RES6K");
    dbShortStart();
}

/*--------------------------------------------------------------------------------------------------
  Free memory used by the database.
--------------------------------------------------------------------------------------------------*/
void dbStop(void)
{
    dbShortStop();
    dbDatabaseStop();
}

