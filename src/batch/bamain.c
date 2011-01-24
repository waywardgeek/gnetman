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
  Netlist manipulation module.
--------------------------------------------------------------------------------------------------*/
#include <stdlib.h>
#include <string.h>
#include "db.h"
#include "htext.h"
#include "tcext.h"
#include "cirext.h"
#include "schext.h"
#include "vrext.h"
#include "pcbext.h"
#include "version.h"

FILE *baFile;

static char *baExecutableName;
static char *baRcFileName;
static bool baInteractive;
static bool baNoInit;

/*--------------------------------------------------------------------------------------------------
  Process arguments, performing the requested actions.
--------------------------------------------------------------------------------------------------*/
static void usage (
   const char *format,
   ...)
{
   va_list ap;
   char *buff;

   if(format != NULL) {
       va_start(ap, format);
       buff = utVsprintf((char *)format, ap);
       va_end(ap);
       utLogMessage("%s", buff);
   }
   utLogMessage("Usage: %s [options] [tcl scripts]\n"
      "   -d <level> -- Sets debug level.  Valid values are 0-3\n"
      "   -c <sch file> -- Creates a top level SPICE file with a .cir extension\n"
      "   -C <sch file> -- Creates a subcircuit SPICE file with a .cir extension\n"
      "   -g <sch file>  --  Generates a block symbol for the schematic\n"
      "   -h -- Print this command summary\n"
      "   -i -- Enter an interactive TCL shell after processing command line\n"
      "   -n -- Do not read any system and user rc files\n"
      "   -p <sch file> -- Creates a PCB compatible netlist file a .net extension\n"
      "   -r <TCL file> -- Execute this TCL file after the system rc file and the \n"
      "                    ${HOME}" UTDIRSEP_STRING ".gEDA" UTDIRSEP_STRING "gnetmanrc.tcl\n"
      "   -st <spice type> -- Choose a spice target format: pspice, hspice, tclspice,\n"
      "                      cdl, or ltspice\n"
      "   -sl <length> -- Set the max line length for SPICE output.  If 0, no line breaks are"
      " added\n"
      "   -v <sch file> -- Creates a Verilog file with a .v extension\n"
      "After processing arguments, any TCL files listed on the command line are\n"
      "executed.\n"
      "This is version %s, compiled on %s\n"
      , baExecutableName, utGetVersion(), utGetCompileTime());
   exit(1);
}

/*--------------------------------------------------------------------------------------------------
  Find the value of the -r flag, if it is specified, so we can run the user's config file before
  processing arguments.
--------------------------------------------------------------------------------------------------*/
static char *findRcFileArgument(
    int argc,
    char **argv)
{
    uint16 xArg;
    const char *optionPtr;

    for(xArg = 1; xArg < argc && argv[xArg][0] == '-'; xArg++) {
        optionPtr = argv[xArg] + 1;
        if(*optionPtr == 'r') {
            xArg++;
            if(xArg < argc) {
                return argv[xArg];
            } else {
                usage("Expecting TCL file after '-%s'", optionPtr);
            }
        }
    }
    return NULL;
}

/*--------------------------------------------------------------------------------------------------
  Process arguments, performing the requested actions.
--------------------------------------------------------------------------------------------------*/
static uint32 processArguments(
    int argc,
    char **argv,
    bool *didSomething)
{
    uint16 xArg;
    const char *optionPtr;
    dbDesign dbCurrentDesign = dbDesignNull;

    baInteractive = false;
    baNoInit = false;
    dbRootSetMaxLineLength(dbTheRoot, 80);
    dbRootSetIncludeTopLevelPorts(dbTheRoot, true);
    for(xArg = 1; xArg < argc && argv[xArg][0] == '-'; xArg++) {
        optionPtr = argv[xArg] + 1;
        switch(*optionPtr) {
        case 'd':
            xArg++;
            if(xArg < argc) {
                utDebugVal = atoi(argv[xArg]);
            } else {
                usage("Expecting debug level after '-%s'", optionPtr);
            }
            break;
        case 'g':
            xArg++;
            if(xArg < argc) {
                if(!schGenerateSymbolFile(argv[xArg])) {
                    utError("Errors creating symbol for %s, exiting...", argv[xArg]);
                }
                *didSomething = true;
            } else {
                usage("Expecting .sch file after '-%s'", optionPtr);
            }
            break;
        case 'c': case 'C':
            xArg++;
            if(xArg < argc) {
                dbCurrentDesign = schReadSchematic(utReplaceSuffix(argv[xArg], ""), argv[xArg],
                    dbDesignNull);
		dbRootSetCurrentDesign(dbTheRoot, dbCurrentDesign);
                if(dbCurrentDesign == dbDesignNull) {
                    utError("Errors reading schematic %s, exiting...", argv[xArg]);
                }
                dbRootSetIncludeTopLevelPorts(dbTheRoot, *optionPtr == 'C');
                if(!cirWriteDesign(dbCurrentDesign, utReplaceSuffix(argv[xArg], ".cir"),
                        dbIncludeTopLevelPorts, dbMaxLineLength, false)) {
                    utError("Errors writing spice netlist, exiting...");
                }
                *didSomething = true;
            } else {
                usage("Expecting .sch file after '-%s'", optionPtr);
            }
            break;
        case 'v':
            xArg++;
            if(xArg < argc) {
                dbCurrentDesign = schReadSchematic(utReplaceSuffix(argv[xArg], ""), argv[xArg],
                    dbDesignNull);
		dbRootSetCurrentDesign(dbTheRoot, dbCurrentDesign);
                if(dbCurrentDesign == dbDesignNull) {
                    utError("Errors reading schematic %s, exiting...", argv[xArg]);
                }
                if(!vrWriteDesign(dbCurrentDesign, utReplaceSuffix(argv[xArg], ".v"), false)) {
                    utError("Errors writing spice netlist, exiting...");
                }
                *didSomething = true;
            } else {
                usage("Expecting .sch file after '-%s'", optionPtr);
            }
            break;
        case 'p':
            xArg++;
            if(xArg < argc) {
                dbCurrentDesign = schReadSchematic(utReplaceSuffix(argv[xArg], ""), argv[xArg],
                    dbDesignNull);
		dbRootSetCurrentDesign(dbTheRoot, dbCurrentDesign);
                if(dbCurrentDesign == dbDesignNull) {
                    utError("Errors reading schematic %s, exiting...", argv[xArg]);
                }
                dbDesignExplodeArrayInsts(dbCurrentDesign);
                if(!pcbWriteDesign(dbCurrentDesign, utReplaceSuffix(argv[xArg], ".net"))) {
                    utError("Errors writing PCB netlist, exiting...");
                }
                *didSomething = true;
            } else {
                usage("Expecting .sch file after '-%s'", optionPtr);
            }
            break;
        case 'i':
            baInteractive = true;
            break;
        case 'n':
            baNoInit = true;
            break;
        case 'h':
            usage(NULL);
            break;
        case 'r':
            /*xArg++;*/
            break;
        case 's':
            /* Process SPICE options */
            switch(optionPtr[1]) {
            case 't':
                xArg++;
                if(xArg < argc) {
                    dbRootSetSpiceTarget(dbTheRoot, dbFindSpiceTargetFromName(argv[xArg]));
                    if(dbSpiceTarget == DB_NULL_SPICE_TYPE) {
                        usage("Unrecognized spice type '%s'", argv[xArg]);
                    }
                } else {
                    usage("Expecting .sch file after '-%s'", optionPtr);
                }
                break;
            case 'l':
                xArg++;
                if(xArg < argc) {
                    dbRootSetMaxLineLength(dbTheRoot, atoi(argv[xArg]));
                } else {
                    usage("Expecting line length after '-%s'", optionPtr);
                }
                break;
            default:
                usage("Unrecognised SPICE option '-%s'", optionPtr);
            }
            break;
        default:
            usage("Unrecognised option '-%s'", optionPtr);
        }
    }
    return xArg;
}

/*--------------------------------------------------------------------------------------------------
  Print the status.
--------------------------------------------------------------------------------------------------*/
static void batchStatus(
    char *message)
{
    printf("%s\n", message);
    fflush(stdout);
}

/*--------------------------------------------------------------------------------------------------
  Set directories from command line
--------------------------------------------------------------------------------------------------*/
static bool setDirectories(
    char *executableName)
{
    char *execDirectory, *configDirectory;

    utSetExeFullPath(utExecPath(executableName));
    execDirectory = utDirName(utGetExeFullPath());
    if(execDirectory == NULL) {
        fprintf(stderr, "cannot find executable path");
        return true;
    }
    configDirectory = utSprintf("%s%cshare%cgEDA", utDirName(execDirectory), UTDIRSEP, UTDIRSEP);
    configDirectory = utFullPath(configDirectory);
    utSetConfigDirectory(configDirectory);
    return false;
}

/*--------------------------------------------------------------------------------------------------
  Run the default .tcl configuration files, if they exist.
--------------------------------------------------------------------------------------------------*/
static void runTclConfigFiles(void)
{
    char *sysConfigFile = utFullPath(utSprintf("%s%c%s", utGetConfigDirectory(), UTDIRSEP, 
        "system-gnetmanrc.tcl"));
    char *homeConfigFile,  *localConfigFile;

    if(utAccess(sysConfigFile, "r")) {
        utLogMessage("Running configuration file %s", sysConfigFile);
        tcRunScript(sysConfigFile);
    }
    homeConfigFile = utSprintf("${HOME}%c.gEDA%c%src.tcl", UTDIRSEP, UTDIRSEP, baExecutableName);
    homeConfigFile = utExpandEnvVariables(homeConfigFile);
    if(utAccess(homeConfigFile, "r")) {
        utLogMessage("Running configuration file %s", homeConfigFile);
        tcRunScript(homeConfigFile);
    }
    localConfigFile = utSprintf("%src.tcl", baExecutableName);
    localConfigFile = utExpandEnvVariables(localConfigFile);
    if(utAccess(localConfigFile, "r")) {
        utLogMessage("Running configuration file %s", localConfigFile);
        tcRunScript(localConfigFile);
    }
    if(baRcFileName != NULL) {
        baRcFileName = utExpandEnvVariables(baRcFileName);
        if(utAccess(baRcFileName, "r")) {
            utLogMessage("Running configuration file %s", baRcFileName);
            tcRunScript(baRcFileName);
        } else {
            utError("Unable to find configuration file %s", baRcFileName);
        }
    }
}

/*--------------------------------------------------------------------------------------------------
  Initialize memory.
--------------------------------------------------------------------------------------------------*/
void baStart(void)
{
    dbStart();
    htStart();
    tcStart();
}

/*--------------------------------------------------------------------------------------------------
  Free memory.
--------------------------------------------------------------------------------------------------*/
void baStop(void)
{
    tcStop();
    htStop();
    dbStop();
}

/*--------------------------------------------------------------------------------------------------
  This is the actual main routine.
--------------------------------------------------------------------------------------------------*/
int main(
    int argc,
    char **argv)
{
    int32 xArg;
    bool didSomething = false;
    char *exeName;

    utStart();
    utSetVersion(utSprintf("gnetman_%u", argv[0], SVNVERSION));
    exeName = utReplaceSuffix(utBaseName(argv[0]), "");
    utInitLogFile(utSprintf("%s.log", exeName));
    baExecutableName = utNewA(char, strlen(exeName) + 1);
    strcpy(baExecutableName, exeName);
    if(setDirectories(argv[0])) {
        utWarning("Unable to set directories");
        return 1;
    }
    utSetStatusCallback(batchStatus);
    baStart();
    if(utSetjmp()) {
        utWarning("Exiting due to errors");
        utFree(baExecutableName);
        baStop();
        utStop(true);
        return 1;
    }
    baRcFileName = findRcFileArgument(argc, argv);
    xArg = processArguments(argc, argv, &didSomething);
    if(!baNoInit) {
        runTclConfigFiles();
    }
    while(xArg < argc) {
        if(!tcRunScript(argv[xArg])) {
            utError("Exiting due to errors loading TCL file %s", argv[xArg]);
        }
        xArg++;
        didSomething = true;
    }
    if(baInteractive) {
        tcRunInterpreter();
    }
    baStop();
    if(!didSomething) {
        usage("Nothing to do");
    }
    utFree(baExecutableName);
    utUnsetjmp();
    utStop(true);
    return 0;
}
