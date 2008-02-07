#include <string.h>
#include "db.h"
#include "schext.h"
#include "cirext.h"
#include "vrext.h"
#include "pcbext.h"
#include "geext.h"
#include "atext.h"

extern char *config_dir;

/*--------------------------------------------------------------------------------------------------
  Set the current design.
--------------------------------------------------------------------------------------------------*/
int set_current_design(
    char *designName)
{
    dbDesign design = dbRootFindDesign(dbTheRoot, utSymCreate(designName));

    if(design == dbDesignNull) {
        utWarning("set_current_design: Unable to locate design %s in the database", designName);
        return 0;
    }
    dbRootSetCurrentDesign(dbTheRoot, design);
    dbRootSetCurrentNetlist(dbTheRoot, dbDesignGetRootNetlist(design));
    return 1;
}

/*--------------------------------------------------------------------------------------------------
  Set the current library design.
--------------------------------------------------------------------------------------------------*/
int set_current_library(
    char *libraryName)
{
    dbDesign design = dbRootFindDesign(dbTheRoot, utSymCreate(libraryName));

    if(design == dbDesignNull) {
        utWarning("set_current_library: Unable to locate library %s in the database", libraryName);
        return 0;
    }
    dbCurrentLibrary = design;
    return 1;
}

/*--------------------------------------------------------------------------------------------------
  Set the current netlist.
--------------------------------------------------------------------------------------------------*/
int set_current_netlist(
    char *netlistName)
{
    dbNetlist netlist;
    dbDesign  dbCurrentDesign = dbRootGetCurrentDesign(dbTheRoot);

    if(dbCurrentDesign == dbDesignNull) {
        utWarning("set_current_netlist: no current design in the database");
        return 0;
    }
    netlist = dbDesignFindNetlist(dbCurrentDesign, utSymCreate(netlistName));
    if(netlist == dbNetlistNull) {
        utWarning("set_current_netlist: Unable to locate netlist %s in the database", netlistName);
        return 0;
    }
    dbCurrentNetlist = netlist;
    return 1;
}

/*--------------------------------------------------------------------------------------------------
  Set the root netlist.
--------------------------------------------------------------------------------------------------*/
int set_root_netlist(
    char *netlistName)
{
    dbNetlist netlist;
    dbDesign  dbCurrentDesign = dbRootGetCurrentDesign(dbTheRoot);

    if(dbCurrentDesign == dbDesignNull) {
        utWarning("set_root_netlist: no current design in the database");
        return 0;
    }
    netlist = dbDesignFindNetlist(dbCurrentDesign, utSymCreate(netlistName));
    if(netlist == dbNetlistNull) {
        utWarning("set_root_netlist: Unable to locate netlist %s in the database", netlistName);
        return 0;
    }
    dbDesignSetRootNetlist(dbCurrentDesign, netlist);
    dbCurrentNetlist = netlist;
    return 1;
}

/*--------------------------------------------------------------------------------------------------
  Create a net in the current netlist, if it does not already exist.
--------------------------------------------------------------------------------------------------*/
void create_net(
    char *netName)
{
    utSym netSym = utSymCreate(netName);

    if(dbCurrentNetlist == dbNetlistNull) {
        utWarning("create_net: no current netlist in the database");
        return;
    }
    if(dbNetlistFindNet(dbCurrentNetlist, netSym) == dbNetNull) {
        dbNetCreate(dbCurrentNetlist, netSym);
    }
}

/*--------------------------------------------------------------------------------------------------
  Merge two nets in the current netlist.
--------------------------------------------------------------------------------------------------*/
void merge_net_into_net(
    char *sourceNetName,
    char *destNetName)
{
    dbNet sourceNet, destNet;
    utSym sourceNetSym = utSymCreate(sourceNetName);
    utSym destNetSym = utSymCreate(destNetName);

    if(dbCurrentNetlist == dbNetlistNull) {
        utWarning("merge_net_into_net: no current netlist in the database");
        return;
    }
    sourceNet = dbNetlistFindNet(dbCurrentNetlist, sourceNetSym);
    if(sourceNet == dbNetNull) {
        utWarning("merge_nets: no net named %s in the current netlist", sourceNetName);
        return;
    }
    destNet = dbNetlistFindNet(dbCurrentNetlist, destNetSym);
    if(destNet == dbNetNull) {
        utWarning("merge_net_into_net: no net named %s in the current netlist", destNetName);
        return;
    }
    dbMergeNetIntoNet(sourceNet, destNet);
}

/*--------------------------------------------------------------------------------------------------
  Get the current design.
--------------------------------------------------------------------------------------------------*/
char *get_current_design(void)
{
    dbDesign  dbCurrentDesign = dbRootGetCurrentDesign(dbTheRoot);
    if(dbCurrentDesign == dbDesignNull) {
        utWarning("get_current_design: no current design in the database");
        return "";
    }
    return dbDesignGetName(dbCurrentDesign);
}

/*--------------------------------------------------------------------------------------------------
  Add a component directory to the component path.
--------------------------------------------------------------------------------------------------*/
void component_library(
    char *dirName)
{
    char *newPath;
  
    if(strcmp(dbGschemComponentPath, ".")) {
        newPath = utSprintf(".:%s:%s", dirName, dbGschemComponentPath + 2);
    } else {
        newPath = utSprintf(".:%s", dirName);
    }
    dbRootResizeGschemComponentPaths(dbTheRoot, strlen(newPath) + 1);
    strcpy(dbGschemComponentPath, newPath);
}

/*--------------------------------------------------------------------------------------------------
  Add a schematic source directory to the source path.
--------------------------------------------------------------------------------------------------*/
void source_library(
    char *dirName)
{
    char *newPath;
  
    if(strcmp(dbGschemSourcePath, ".")) {
        newPath = utSprintf(".:%s:%s", dirName, dbGschemSourcePath + 2);
    } else {
        newPath = utSprintf(".:%s", dirName);
    }
    dbRootResizeGschemSourcePaths(dbTheRoot, strlen(newPath) + 1);
    strcpy(dbGschemSourcePath, newPath);
}

/*--------------------------------------------------------------------------------------------------
  reset the source library to ".".
--------------------------------------------------------------------------------------------------*/
void reset_source_library(void)
{
    strcpy(dbGschemSourcePath, ".");
}

/*--------------------------------------------------------------------------------------------------
  reset the compoent library to ".".
--------------------------------------------------------------------------------------------------*/
void reset_component_library(void)
{
    strcpy(dbGschemComponentPath, ".");
}

/*--------------------------------------------------------------------------------------------------
  Set the current SPICE target.
--------------------------------------------------------------------------------------------------*/
void set_spice_target(
    char *spiceType)
{
    dbSpiceTargetType target = dbFindSpiceTargetFromName(spiceType);

    if(target == DB_NULL_SPICE_TYPE) {
        utWarning("Unknown spice type '%s': try one of pspice, hspice, tclspice, or ltspice",
            spiceType);
        return;
    }
    dbSpiceTarget = target;
}

/*--------------------------------------------------------------------------------------------------
  Add a new device type for spice netlisting.
--------------------------------------------------------------------------------------------------*/
void add_spice_device(
    char *device)
{
    dbDevspec devspec = dbFindCurrentDevspec();
    char *newDeviceString;
  
    newDeviceString = utSprintf("%s%s\n", dbDevspecGetString(devspec), device);
    dbDevspecResizeStrings(devspec, strlen(newDeviceString) + 1);
    strcpy(dbDevspecGetString(devspec), newDeviceString);
}

/*--------------------------------------------------------------------------------------------------
  Tell the parser if dollar sign should be considered as a comment
--------------------------------------------------------------------------------------------------*/
void set_dollar_as_comment(void)
{
    dbDevspec devspec = dbFindCurrentDevspec();
    dbDevspecSetDollarAsComment(devspec, true);
}


/*--------------------------------------------------------------------------------------------------
  Tell the parser if dollar sign should not be considered as a comment
--------------------------------------------------------------------------------------------------*/
void unset_dollar_as_comment(void)
{
    dbDevspec devspec = dbFindCurrentDevspec();
    dbDevspecSetDollarAsComment(devspec, false);
}



/*--------------------------------------------------------------------------------------------------
  Tell the parser if dollar sign should not be considered as a comment
--------------------------------------------------------------------------------------------------*/
int get_dollar_as_comment(void)
{
    dbDevspec devspec = dbFindCurrentDevspec();
    return dbDevspecDollarAsComment(devspec);
}




/*--------------------------------------------------------------------------------------------------
  Write out the symbol for the schematic.
--------------------------------------------------------------------------------------------------*/
int create_default_symbol(
    char *schemName)
{
    int passed;
  
    if(!utAccess(schemName, "r")) {
        utWarning("Unable to read schematic %s", schemName);
        return 0;
    }
    passed = schGenerateSymbolFile(schemName);
    if(!passed) {
        return 0;
    }
    return 1;
}

/*--------------------------------------------------------------------------------------------------
  Setting this flag to false causes the SPICE netlister to not write out the top level ports.
--------------------------------------------------------------------------------------------------*/
void set_include_top_level_ports(
    int value)
{
    dbIncludeTopLevelPorts = value;
}

/*--------------------------------------------------------------------------------------------------
  The max length of a line in our output format.
--------------------------------------------------------------------------------------------------*/
void set_max_line_length(
    int value)
{
    dbMaxLineLength  = value;
}

/*--------------------------------------------------------------------------------------------------
  Write a spice netlist.
--------------------------------------------------------------------------------------------------*/
int write_netlist(
    char *format,
    char *fileName)
{
    dbNetlistFormat netlistFormat = dbFindNetlistFormat(format);
    bool passed = true;
    dbDesign  dbCurrentDesign = dbRootGetCurrentDesign(dbTheRoot);

    if(dbCurrentDesign == dbDesignNull) {
        utWarning("write_spice_netlist: No current design in the database");
        return 0;
    }
    if(netlistFormat == DB_NOFORMAT) {
        utWarning("Unknown input format specified: %s", format);
        return 0;
    }
    switch(netlistFormat) {
    case DB_VERILOG:
        passed = vrWriteDesign(dbCurrentDesign, fileName, false);
        break;
    case DB_SPICE:
        passed = cirWriteDesign(dbCurrentDesign, fileName, dbIncludeTopLevelPorts,
            dbMaxLineLength, false);
        break;
    case DB_PCB:
        passed = pcbWriteDesign(dbCurrentDesign, fileName);
        break;
    default:
        utExit("Unknown netlist format");
    }
    if(!passed) {
        return 0;
    }
    return 1;
}

/*--------------------------------------------------------------------------------------------------
  Read a netlist.  The format option specifies the format type, ie spice or verilog.
--------------------------------------------------------------------------------------------------*/
int read_netlist(
    char *format,
    char *designName,
    char *fileName)
{
    dbNetlistFormat netlistFormat = dbFindNetlistFormat(format);
    dbDesign  dbCurrentDesign = dbDesignNull;

    if(netlistFormat == DB_NOFORMAT) {
        utWarning("Unknown input format specified: %s", format);
        return 0;
    }
    if(!utAccess(fileName, "r")) {
        utWarning("Unable to read %s", fileName);
        return 0;
    }
    switch(netlistFormat) {
    case DB_VERILOG:
        dbCurrentDesign = vrReadDesign(designName, fileName, dbCurrentLibrary);
        break;
    case DB_SPICE:
        dbCurrentDesign = cirReadDesign(designName, fileName, dbCurrentLibrary);
        break;
    case DB_SCHEMATIC:
        dbCurrentDesign = schReadSchematic(designName, fileName, dbCurrentLibrary);
        break;
    case DB_PCB:
        dbCurrentDesign = pcbReadDesign(designName, fileName, dbCurrentLibrary);
        break;
    default:
        utExit("Unknown netlist format");
    }
    if(dbCurrentDesign == dbDesignNull) {
        return 0;
    }
    dbCurrentNetlist = dbDesignGetRootNetlist(dbCurrentDesign);
    dbRootSetCurrentNetlist(dbTheRoot, dbCurrentNetlist);
    return 1;
}

/*--------------------------------------------------------------------------------------------------
  Read a library netlist.  The only difference is that all the netlists read are marked as being
  library components, so their guts aren't written out later if we write out the netlist.
--------------------------------------------------------------------------------------------------*/
int read_library(
    char *format,
    char *libraryName,
    char *fileName)
{
    dbNetlistFormat netlistFormat = dbFindNetlistFormat(format);

    if(netlistFormat == DB_NOFORMAT) {
        utWarning("Unknown input format specified: %s", format);
        return 0;
    }
    if(!utAccess(fileName, "r")) {
        utWarning("Unable to read %s", fileName);
        return 0;
    }
    switch(netlistFormat) {
    case DB_VERILOG:
        dbCurrentLibrary = vrReadDesign(libraryName, fileName, dbCurrentLibrary);
        break;
    case DB_SPICE:
        dbCurrentLibrary = cirReadDesign(libraryName, fileName, dbCurrentLibrary);
        break;
    case DB_SCHEMATIC:
        dbCurrentLibrary = schReadSchematic(libraryName, fileName, dbCurrentLibrary);
        break;
    default:
        utExit("Unknown netlist format");
    }
    if(dbCurrentLibrary == dbDesignNull) {
        return 0;
    }
    return 1;
}

/*--------------------------------------------------------------------------------------------------
  Write a library.
--------------------------------------------------------------------------------------------------*/
int write_library(
    char *format,
    char *fileName)
{
    dbNetlistFormat netlistFormat = dbFindNetlistFormat(format);
    bool passed = true;
    dbDesign  dbCurrentDesign = dbRootGetCurrentDesign(dbTheRoot);

    if(dbCurrentDesign == dbDesignNull) {
        utWarning("write_spice_netlist: No current design in the database");
        return 0;
    }
    if(netlistFormat == DB_NOFORMAT) {
        utWarning("Unknown input format specified: %s", format);
        return 0;
    }
    switch(netlistFormat) {
    case DB_VERILOG:
        passed = vrWriteDesign(dbCurrentDesign, fileName, true);
        break;
    case DB_SPICE:
        passed = cirWriteDesign(dbCurrentDesign, fileName, dbIncludeTopLevelPorts,
            dbMaxLineLength, true);
        break;
    default:
        utExit("Unknown netlist format");
    }
    if(!passed) {
        return 0;
    }
    return 1;
}

/*--------------------------------------------------------------------------------------------------
  Cause primitive components to be generated into actual library devices.
--------------------------------------------------------------------------------------------------*/
int generate_devices(void)
{
    dbDesign  dbCurrentDesign = dbRootGetCurrentDesign(dbTheRoot);

    if(dbCurrentDesign == dbDesignNull) {
        utWarning("genereate_devices: No design present in database");
        return  0;
    }
    if(geGenerateDevices()) {
        return 1;
    }
    return 0;
}

/*--------------------------------------------------------------------------------------------------
  Connvert power instances into globals.
--------------------------------------------------------------------------------------------------*/
void convert_power_insts_to_globals(void)
{
    dbDesign  dbCurrentDesign = dbRootGetCurrentDesign(dbTheRoot);

    if(dbCurrentDesign == dbDesignNull) {
        utWarning("convert_power_insts_to_globals: no current design");
    } else {
        dbDesignConvertPowerInstsToGlobals(dbCurrentDesign);
    }
}

/*--------------------------------------------------------------------------------------------------
  Thread a global signal from the root netlist all the way down to where it is used.  Try to use
  mport names that match the global name.
--------------------------------------------------------------------------------------------------*/
void thread_global_through_hierarchy(
    char *globalName,
    int createTopLevelPorts)
{
    dbGlobal global;
    dbDesign  dbCurrentDesign = dbRootGetCurrentDesign(dbTheRoot);

    if(dbCurrentDesign == dbDesignNull) {
        utWarning("thread_global_through_hierarchy: no current design");
    } else {
        global = dbDesignFindGlobal(dbCurrentDesign, utSymCreate(globalName));
        if(global == dbGlobalNull) {
            utWarning("thread_globals_through_hierarchy: no global '%s' found in design",
                globalName);
        } else {
            dbThreadGlobalThroughHierarchy(global, createTopLevelPorts);
        }
    }
}

/*--------------------------------------------------------------------------------------------------
  Thread global signals from the root netlist all the way down to where they are used.  Try to use
  mport names that match the global names.
--------------------------------------------------------------------------------------------------*/
void thread_globals_through_hierarchy(
    int createTopLevelPorts)
{
    dbDesign  dbCurrentDesign = dbRootGetCurrentDesign(dbTheRoot);

    if(dbCurrentDesign == dbDesignNull) {
        utWarning("thread_globals_through_hierarchy: no current design");
    } else {
        dbThreadGlobalsThroughHierarchy(dbCurrentDesign, createTopLevelPorts);
    }
}

/*--------------------------------------------------------------------------------------------------
  Rename the global.
--------------------------------------------------------------------------------------------------*/
void rename_global(
    char *globalName,
    char *newGlobalName)
{
    dbGlobal global;
    dbDesign  dbCurrentDesign = dbRootGetCurrentDesign(dbTheRoot);

    if(dbCurrentDesign == dbDesignNull) {
        utWarning("rename_global: no current design");
    } else {
        global = dbDesignFindGlobal(dbCurrentDesign, utSymCreate(globalName));
        if(global == dbGlobalNull) {
            utWarning("rename_global: no global named %s in current design", globalName);
        } else {
            dbGlobalRename(global, utSymCreate(newGlobalName));
        }
    }
}

/*--------------------------------------------------------------------------------------------------
  Rename the netlist.
--------------------------------------------------------------------------------------------------*/
void rename_netlist(
    char *netlistName,
    char *newNetlistName)
{
    dbNetlist netlist;
    dbDesign  dbCurrentDesign = dbRootGetCurrentDesign(dbTheRoot);

    if(dbCurrentDesign == dbDesignNull) {
        utWarning("rename_netlist: no current design");
    } else {
        netlist = dbDesignFindNetlist(dbCurrentDesign, utSymCreate(netlistName));
        if(netlist == dbNetlistNull) {
            utWarning("rename_netlist: no netlist named %s in current design", netlistName);
        } else {
            dbNetlistRename(netlist, utSymCreate(newNetlistName));
        }
    }
}

/*--------------------------------------------------------------------------------------------------
  Set all names in the netlist to  upper case.
--------------------------------------------------------------------------------------------------*/
int make_netlists_upper_case(
    char *designName)
{
    dbDesign design = dbRootFindDesign(dbTheRoot, utSymCreate(designName));

    if(design == dbDesignNull) {
        utWarning("make_netlist_upper_case: could not find design %s", designName);
        return 0;
    }
    dbDesignMakeNetlistNamesUpperCase(design);
    return 1;
}

/*--------------------------------------------------------------------------------------------------
  Set an attribute on a netlist.
--------------------------------------------------------------------------------------------------*/
void set_netlist_value(
    char *netlistName,
    char *propName,
    char *value)
{
    dbNetlist netlist;
    dbDesign  dbCurrentDesign = dbRootGetCurrentDesign(dbTheRoot);

    if(dbCurrentDesign == dbDesignNull) {
        utWarning("set_netlist_value: no current design");
    } else {
        netlist = dbDesignFindNetlist(dbCurrentDesign, utSymCreate(netlistName));
        if(netlist == dbNetlistNull) {
            utWarning("set_netlist_value: no netlist named %s in current design", netlistName);
        } else {
            dbNetlistSetValue(netlist, utSymCreate(propName), utSymCreate(value));
        }
    }
}

/*--------------------------------------------------------------------------------------------------
  Get an attribute of an netlist.
--------------------------------------------------------------------------------------------------*/
char *get_netlist_value(
    char *netlistName,
    char *propName)
{
    dbNetlist netlist;
    utSym value;
    dbDesign  dbCurrentDesign = dbRootGetCurrentDesign(dbTheRoot);

    if(dbCurrentDesign == dbDesignNull) {
        utWarning("get_netlist_value: no current design");
        return "";
    }
    netlist = dbDesignFindNetlist(dbCurrentDesign, utSymCreate(netlistName));
    if(netlist == dbNetlistNull) {
        utWarning("get_netlist_value: no netlist named %s in current design", netlistName);
        return "";
    }
    value = dbNetlistGetValue(netlist, utSymCreate(propName));
    if(value == utSymNull) {
        return "";
    }
    return utSymGetName(value);
}

/*--------------------------------------------------------------------------------------------------
  Set an attribute on a inst.
--------------------------------------------------------------------------------------------------*/
void set_inst_value(
    char *instName,
    char *propName,
    char *value)
{
    dbInst inst;
    dbDesign  dbCurrentDesign = dbRootGetCurrentDesign(dbTheRoot);

    if(dbCurrentDesign == dbDesignNull) {
        utWarning("set_inst_value: no current design");
        return;
    }
    if(dbCurrentNetlist == dbNetlistNull) {
        utWarning("set_inst_value: no current netlist in the database");
        return;
    }
    inst = dbNetlistFindInst(dbCurrentNetlist, utSymCreate(instName));
    if(inst == dbInstNull) {
        utWarning("set_inst_value: no inst named %s in current netlist", instName);
    } else {
        dbInstSetValue(inst, utSymCreate(propName), utSymCreate(value));
    }
}

/*--------------------------------------------------------------------------------------------------
  Get an attribute of an inst.
--------------------------------------------------------------------------------------------------*/
char *get_inst_value(
    char *instName,
    char *propName)
{
    dbInst inst;
    utSym value;
    dbDesign  dbCurrentDesign = dbRootGetCurrentDesign(dbTheRoot);

    if(dbCurrentDesign == dbDesignNull) {
        utWarning("get_inst_value: no current design");
        return "";
    }
    if(dbCurrentNetlist == dbNetlistNull) {
        utWarning("get_inst_value: no current netlist in the database");
        return "";
    }
    inst = dbNetlistFindInst(dbCurrentNetlist, utSymCreate(instName));
    if(inst == dbInstNull) {
        utWarning("get_inst_value: no inst named %s in current netlist", instName);
        return "";
    }
    value = dbInstGetValue(inst, utSymCreate(propName));
    if(value == utSymNull) {
        return "";
    }
    return utSymGetName(value);
}

/*--------------------------------------------------------------------------------------------------
  Set an attribute on a net.
--------------------------------------------------------------------------------------------------*/
void set_net_value(
    char *netName,
    char *propName,
    char *value)
{
    dbNet net;
    dbDesign  dbCurrentDesign = dbRootGetCurrentDesign(dbTheRoot);

    if(dbCurrentDesign == dbDesignNull) {
        utWarning("set_net_value: no current design");
        return;
    }
    if(dbCurrentNetlist == dbNetlistNull) {
        utWarning("set_net_value: no current netlist in the database");
        return;
    }
    net = dbNetlistFindNet(dbCurrentNetlist, utSymCreate(netName));
    if(net == dbNetNull) {
        utWarning("set_net_value: no net named %s in current netlist", netName);
    } else {
        dbNetSetValue(net, utSymCreate(propName), utSymCreate(value));
    }
}

/*--------------------------------------------------------------------------------------------------
  Get an attribute of an net.
--------------------------------------------------------------------------------------------------*/
char *get_net_value(
    char *netName,
    char *propName)
{
    dbNet net;
    utSym value;
    dbDesign  dbCurrentDesign = dbRootGetCurrentDesign(dbTheRoot);

    if(dbCurrentDesign == dbDesignNull) {
        utWarning("get_net_value: no current design");
        return "";
    }
    if(dbCurrentNetlist == dbNetlistNull) {
        utWarning("get_net_value: no current netlist in the database");
        return "";
    }
    net = dbNetlistFindNet(dbCurrentNetlist, utSymCreate(netName));
    if(net == dbNetNull) {
        utWarning("get_net_value: no net named %s in current netlist", netName);
        return "";
    }
    value = dbNetGetValue(net, utSymCreate(propName));
    if(value == utSymNull) {
        return "";
    }
    return utSymGetName(value);
}

/*--------------------------------------------------------------------------------------------------
  Explode array instances into individual instances.
--------------------------------------------------------------------------------------------------*/
void explode_instance_arrays(void)
{
    dbDesign  dbCurrentDesign = dbRootGetCurrentDesign(dbTheRoot);

    if(dbCurrentDesign == dbDesignNull) {
        utWarning("explode_instance_arrays: no current design");
        return;
    }
    dbDesignExplodeArrayInsts(dbCurrentDesign);
}

/*--------------------------------------------------------------------------------------------------
  Get the first instance of the current netlist.
--------------------------------------------------------------------------------------------------*/
char *get_first_inst(void)
{
    dbInst inst;

    if(dbCurrentNetlist == dbNetlistNull) {
        utWarning("get_first_inst: no current netlist in the database");
        return "";
    }
    dbForeachNetlistInst(dbCurrentNetlist, inst) {
        if(dbInstGetType(inst) != DB_FLAG) {
            return dbInstGetUserName(inst);
        }
    } dbEndNetlistInst;
    return "";
}

/*--------------------------------------------------------------------------------------------------
  Get the next instance of the current netlist.
--------------------------------------------------------------------------------------------------*/
char *get_next_inst(
    char *instName)
{
    dbInst inst;

    if(dbCurrentNetlist == dbNetlistNull) {
        utWarning("get_next_inst: no current netlist in the database");
        return "";
    }
    inst = dbNetlistFindInst(dbCurrentNetlist, utSymCreate(instName));
    if(inst == dbInstNull) {
        utWarning("get_next_inst: no instance named %s in the database", instName);
        return "";
    }
    utDo {
        inst = dbInstGetNextNetlistInst(inst);
    } utWhile(inst != dbInstNull) {
        if(dbInstGetType(inst) != DB_FLAG) {
            return dbInstGetUserName(inst);
        }
    } utRepeat;
    return "";
}

/*--------------------------------------------------------------------------------------------------
  Get the first net of the current netlist.
--------------------------------------------------------------------------------------------------*/
char *get_first_net(void)
{
    dbNet net;

    if(dbCurrentNetlist == dbNetlistNull) {
        utWarning("get_first_net: no current netlist in the database");
        return "";
    }
    net = dbNetlistGetFirstNet(dbCurrentNetlist);
    if(net == dbNetNull) {
        return "";
    }
    return dbNetGetName(net);
}

/*--------------------------------------------------------------------------------------------------
  Get the next net of the current netlist.
--------------------------------------------------------------------------------------------------*/
char *get_next_net(
    char *netName)
{
    dbNet net;

    if(dbCurrentNetlist == dbNetlistNull) {
        utWarning("get_next_net: no current netlist in the database");
        return "";
    }
    net = dbNetlistFindNet(dbCurrentNetlist, utSymCreate(netName));
    if(net == dbNetNull) {
        utWarning("get_next_net: no net named %s in the database", netName);
        return "";
    }
    net = dbNetGetNextNetlistNet(net);
    if(net == dbNetNull) {
        return "";
    }
    return dbNetGetName(net);
}

/*--------------------------------------------------------------------------------------------------
  Get the first netlist of the current design.
--------------------------------------------------------------------------------------------------*/
char *get_first_netlist(void)
{
    dbNetlist netlist;
    dbDesign  dbCurrentDesign = dbRootGetCurrentDesign(dbTheRoot);

    if(dbCurrentDesign == dbDesignNull) {
        utWarning("get_first_netlist: no current design in the database");
        return "";
    }
    dbForeachDesignNetlist(dbCurrentDesign, netlist) {
        if(dbNetlistGetType(netlist) == DB_SUBCIRCUIT) {
            return dbNetlistGetName(netlist);
        }
    } dbEndDesignNetlist;
    return "";
}

/*--------------------------------------------------------------------------------------------------
  Get the next netlist of the current design.
--------------------------------------------------------------------------------------------------*/
char *get_next_netlist(
    char *netlistName)
{
    dbNetlist netlist;
    dbDesign  dbCurrentDesign = dbRootGetCurrentDesign(dbTheRoot);

    if(dbCurrentDesign == dbDesignNull) {
        utWarning("get_next_netlist: no current design in the database");
        return "";
    }
    netlist = dbDesignFindNetlist(dbCurrentDesign, utSymCreate(netlistName));
    if(netlist == dbNetlistNull) {
        utWarning("get_next_netlist: no netlist named %s in the database", netlistName);
        return "";
    }
    utDo {
        netlist = dbNetlistGetNextDesignNetlist(netlist);
    } utWhile(netlist != dbNetlistNull) {
        if(dbNetlistGetType(netlist) == DB_SUBCIRCUIT) {
            return dbNetlistGetName(netlist);
        }
    } utRepeat;
    return "";
}

/*--------------------------------------------------------------------------------------------------
  Get the internal netlist name of the instance.
--------------------------------------------------------------------------------------------------*/
char *get_inst_internal_netlist(
    char *instName)
{
    dbInst inst;

    if(dbCurrentNetlist == dbNetlistNull) {
        utWarning("get_inst_internal_netlist: no current netlist in the database");
        return "";
    }
    inst = dbNetlistFindInst(dbCurrentNetlist, utSymCreate(instName));
    if(inst == dbInstNull) {
        utWarning("get_inst_internal_netlist: no instance named %s in the database", instName);
        return "";
    }
    return dbNetlistGetName(dbInstGetInternalNetlist(inst));
}

/*--------------------------------------------------------------------------------------------------
  Determine if a net by the name exists in the current netlist.
--------------------------------------------------------------------------------------------------*/
int net_exists(
    char *netName)
{
    if(dbCurrentNetlist == dbNetlistNull) {
        utWarning("net_exists: no current netlist in the database");
        return false;
    }
    return dbNetlistFindNet(dbCurrentNetlist, utSymCreate(netName)) != dbNetNull;
}

/*--------------------------------------------------------------------------------------------------
  Determine if a inst by the name exists in the current netlist.
--------------------------------------------------------------------------------------------------*/
int inst_exists(
    char *instName)
{
    if(dbCurrentNetlist == dbNetlistNull) {
        utWarning("inst_exists: no current netlist in the database");
        return false;
    }
    return dbNetlistFindInst(dbCurrentNetlist, utSymCreate(instName)) != dbInstNull;
}

/*--------------------------------------------------------------------------------------------------
  Compute the sum of the expression on all netlist ports.
--------------------------------------------------------------------------------------------------*/
void report_port_sums(
    char *deviceType,
    char *pinType,
    char *expression)
{
    dbDesign  dbCurrentDesign = dbRootGetCurrentDesign(dbTheRoot);
    atReportDesignSums(dbCurrentDesign, utSymCreate(deviceType), utSymCreate(pinType), expression);
}

/*--------------------------------------------------------------------------------------------------
  Compute the sum of the expression on all netlist ports.  The pin list is a string consisting of
  pairs of space separated words representing device types and pin types.  For example "M S M D"
  represents both the source and drain of a mosfet.
--------------------------------------------------------------------------------------------------*/
void report_portlist_sums(
    char *pinList,
    char *expression)
{
    dbDesign  dbCurrentDesign = dbRootGetCurrentDesign(dbTheRoot);
    atReportDesignSumsOverPins(dbCurrentDesign, expression, pinList);
}

/*--------------------------------------------------------------------------------------------------
  Setting this causes library netlists to replace the user netlists.  This is useful when the user
  can't easily remove the guts of library components when netlisting.
--------------------------------------------------------------------------------------------------*/
void set_library_wins(
    int value)
{
    dbLibraryWins = value? true : false;
}

/*--------------------------------------------------------------------------------------------------
  Temp hack to set resistor names.
--------------------------------------------------------------------------------------------------*/
void set_resistor_names(
    char *res250,
    char *res6k)
{
    geRES250Sym = utSymCreate(res250);
    geRES6KSym = utSymCreate(res6k);
}

/*--------------------------------------------------------------------------------------------------
  Save the database in binary format.
--------------------------------------------------------------------------------------------------*/
void save_database(
    char *fileName)
{
    FILE *file = fopen(fileName, "wb");

    if(file == NULL) {
        utWarning("save_database: Unable to open file %s for writing");
        return;
    }
    utLogMessage("Saving database to %s", fileName);
    utSaveTextDatabase(file);
    fclose(file);
}

/*--------------------------------------------------------------------------------------------------
  Load the database in binary format.
--------------------------------------------------------------------------------------------------*/
void load_database(
    char *fileName)
{
    FILE *file = fopen(fileName, "rb");

    if(file == NULL) {
        utWarning("load_database: Unable to open file %s for writing");
        return;
    }
    utLogMessage("Reading database from %s", fileName);
    utLoadTextDatabase(file);
    dbTheRoot = dbFirstRoot();
    dbGraphicalSym = utSymCreate("graphical");
    geRES250Sym = utSymCreate("RES250");
    geRES6KSym = utSymCreate("RES6K");
    fclose(file);
}
