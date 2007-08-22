/*--------------------------------------------------------------------------------------------------
  Portions of this file were copied from gnetlist source code.
--------------------------------------------------------------------------------------------------*/
#include <string.h>
#include "gu.h"
#include "schext.h"
#include "cirext.h"

/* The following code was contributed by thi (with formating changes
 * by Ales) Thanks!  */
/* Later updated by spe */
/* Later included here by Bill Cox */

/* This `load()' is modeled after libguile/load.c, load().
 * Additionally, the most recent form read is saved in case something
 * goes wrong. */
static SCM guMostRecentlyReadForm = SCM_BOOL_F;

/*--------------------------------------------------------------------------------------------------
  Load and evaluate a scheme file.
--------------------------------------------------------------------------------------------------*/
static SCM load(
    void *data)
{
    SCM loadPort = (SCM)data;
    SCM form;
    bool eofFound = false;

    while(!eofFound) {
        form = scm_read(loadPort);
        if (SCM_EOF_OBJECT_P(form)) {
            eofFound = true;
        } else {
          guMostRecentlyReadForm = form;
          scm_eval_x(form, scm_current_module());
        }
    }
    guMostRecentlyReadForm = SCM_BOOL_F;
    return SCM_BOOL_T;
}

/*--------------------------------------------------------------------------------------------------
  Report scheme errors.
--------------------------------------------------------------------------------------------------*/
static SCM loadErrorHandler(
    void *data,
    SCM tag,
    SCM throw_args)
{
    SCM curOut = scm_current_output_port();
    SCM loadPort = (SCM)data;
    SCM fileName = scm_port_filename(loadPort);

    /*
     * If misc-error the column and line pointers points
     * to end of file. Not necessary to confuse user.
     */
    if(!gh_eq_p(tag, gh_symbol2scm("misc-error"))) {
        scm_display(scm_makfrom0str("Error : "), curOut);
        scm_display(tag, curOut);
        scm_display(scm_makfrom0str(" [C:"), curOut);
        scm_display(scm_port_column(loadPort), curOut);
        scm_display(scm_makfrom0str(" L:"), curOut);
        scm_display(scm_port_line(loadPort), curOut);
        scm_display(scm_makfrom0str("]"), curOut);
    } else {
        scm_display(scm_makfrom0str("Probably parenthesis mismatch"), curOut);
    }
    scm_display(scm_makfrom0str(" in "), curOut);
    scm_display(fileName, curOut);
    scm_newline(curOut);
    if(guMostRecentlyReadForm != SCM_BOOL_F) {
        scm_display(scm_makfrom0str("Most recently read form: "), curOut);
        scm_display(guMostRecentlyReadForm, curOut);
        scm_newline(curOut);
    }
    return SCM_BOOL_F;
}

/*--------------------------------------------------------------------------------------------------
  Run a scheme file.
--------------------------------------------------------------------------------------------------*/
bool guLoadFile(
    char *fileName)
{
    SCM port;
    SCM result;

    if(fileName == NULL) {
        return false;
    }
    if(!utAccess(fileName, "r")) {
        utLogMessage("Could not open file %s", fileName);
        return false;
    }
    port = scm_open_file(scm_makfrom0str(fileName), scm_makfromstr("r", (scm_sizet) sizeof(char), 0));
    result = gh_catch(SCM_BOOL_T, (scm_catch_body_t)load, (void *)port,
        (scm_catch_handler_t)loadErrorHandler, (void *)port);
    scm_close_port(port);
    return result == SCM_BOOL_T;
}

/*--------------------------------------------------------------------------------------------------
  Return a string from a scheme object.  This returns a temporary buffer that does not need to
  be freed.
--------------------------------------------------------------------------------------------------*/
static char *scmString(
    SCM scmstring)
{
    char *buf = gh_scm2newstr(scmstring, NULL);
    char *string = utCopyString(buf);

    free(buf);
    return string;
}

/*--------------------------------------------------------------------------------------------------
  Add a component directory to the component path.
--------------------------------------------------------------------------------------------------*/
static SCM addComponentLibrary(
    SCM directory)
{
    char *dirName = scmString(directory);
    char *newPath;
  
    newPath = utSprintf("%s:%s", dbGschemComponentPath, dirName);
    utResizeArray(dbGschemComponentPath, strlen(newPath) + 1);
    strcpy(dbGschemComponentPath, newPath);
    return SCM_BOOL_T;
}

/*--------------------------------------------------------------------------------------------------
  Add a schematic source directory to the source path.
--------------------------------------------------------------------------------------------------*/
static SCM addSourceLibrary(
    SCM directory)
{
    char *dirName = scmString(directory);
    char *newPath;
  
    newPath = utSprintf("%s:%s", dbGschemSourcePath, dirName);
    utResizeArray(dbGschemSourcePath, strlen(newPath) + 1);
    strcpy(dbGschemSourcePath, newPath);
    return SCM_BOOL_T;
}

/*--------------------------------------------------------------------------------------------------
  reset the source library to ".".
--------------------------------------------------------------------------------------------------*/
static SCM resetSourceLibrary(void)
{
    strcpy(dbGschemSourcePath, ".");
    return SCM_BOOL_T;
}

/*--------------------------------------------------------------------------------------------------
  reset the compoent library to ".".
--------------------------------------------------------------------------------------------------*/
static SCM resetComponentLibrary(void)
{
    strcpy(dbGschemComponentPath, ".");
    return SCM_BOOL_T;
}

/*--------------------------------------------------------------------------------------------------
  Add a new device type for spice netlisting.
--------------------------------------------------------------------------------------------------*/
static SCM addSpiceDevice(
    SCM scmdevice)
{
    char *device = scmString(scmdevice);
    char *newDeviceString = utSprintf("%s\n%s", dbSpiceDeviceString, device);
  
    utResizeArray(dbSpiceDeviceString, strlen(newDeviceString) + 1);
    strcpy(dbSpiceDeviceString, newDeviceString);
    return SCM_BOOL_T;
}

/*--------------------------------------------------------------------------------------------------
  Write out the symbol for the schematic.
--------------------------------------------------------------------------------------------------*/
static SCM createDefaultSymbol(
    SCM schem)
{
    char *schemName = scmString(schem);
    bool passed;
  
    if(!utAccess(schemName, "r")) {
        utWarning("Unable to read schematic %s", schemName);
        return SCM_BOOL_F;
    }
    passed = schGenerateSymbolFile(schemName);
    if(!passed) {
        return SCM_BOOL_F;
    }
    return SCM_BOOL_T;
}

/*--------------------------------------------------------------------------------------------------
  Read a schematic netlist into the database.
--------------------------------------------------------------------------------------------------*/
static SCM readSchematic(
    SCM schem)
{
    char *schemName = scmString(schem);
    bool passed;
  
    if(!utAccess(schemName, "r")) {
        utWarning("Unable to read schematic %s", schemName);
        return SCM_BOOL_F;
    }
    passed = schReadSchematic(schemName) != dbDesignNull;
    if(!passed) {
        return SCM_BOOL_F;
    }
    return SCM_BOOL_T;
}

/*--------------------------------------------------------------------------------------------------
  Write a spice netlist.
--------------------------------------------------------------------------------------------------*/
static SCM writeSpiceNetlist(
    SCM scmdesign,
    SCM scmspice,
    SCM includeTopLevelPorts)
{
    dbDesign design;
    char *designName = scmString(scmdesign);
    char *spiceFileName;
    bool passed;
  
    design = dbRootFindDesign(dbTheRoot, utSymCreate(designName));
    if(design == dbDesignNull) {
        utWarning("Unable to locate design %s in the database", designName);
        return SCM_BOOL_F;
    }
    spiceFileName = scmString(scmspice);
    passed = cirWriteDesign(design, spiceFileName, includeTopLevelPorts != SCM_BOOL_F);
    if(!passed) {
        return SCM_BOOL_F;
    }
    return SCM_BOOL_T;
}

/*--------------------------------------------------------------------------------------------------
  Initialize Guile, register functions, load the rc file.
--------------------------------------------------------------------------------------------------*/
static void registerFunctions(void)
{
    gh_new_procedure0_0("reset-component-library", resetComponentLibrary);
    gh_new_procedure0_0("reset-source-library", resetSourceLibrary);
    gh_new_procedure1_0("component-library", addComponentLibrary);
    gh_new_procedure1_0("source-library", addSourceLibrary);
    gh_new_procedure1_0("create-default-symbol", createDefaultSymbol);
    gh_new_procedure1_0("read-schematic", readSchematic);
    gh_new_procedure3_0("write-spice-netlist", writeSpiceNetlist);
    gh_new_procedure1_0("add-spice-device", addSpiceDevice);
}

/*--------------------------------------------------------------------------------------------------
  Initialize Guile, register functions, load the rc file.
--------------------------------------------------------------------------------------------------*/
bool guStart(
    char *rcFileName)
{
    rcFileName = utExpandEnvVariables(rcFileName);
    registerFunctions();
    return guLoadFile(rcFileName);
}

/*--------------------------------------------------------------------------------------------------
  Shut down Guile, and free any memory allocated.
--------------------------------------------------------------------------------------------------*/
void guStop(void)
{
}

/*--------------------------------------------------------------------------------------------------
  This is a super-stupid thing for a control tool to do... It makes you call it's main entry
  routine and then it calls your main routing.  In environments where we don't even have a main,
  like in Windows, this will be a problem.  It makes the whole thought of using Guile barely
  tollerable.  Be careful... this routine WILL NEVER RETURN!
--------------------------------------------------------------------------------------------------*/
void guEnterGuileMainRoutine(
    int argc,
    char **argv,
    void myRealMainRoutine(int argc, char *argv[]))
{
    /* disable the deprecated warnings in guile 1.6.3 */
    /* Eventually the warnings will need to be fixed */
    /* if(getenv("GUILE_WARN_DEPRECATED")==NULL) { */
    if(utGetEnvironmentVariable("GUILE_WARN_DEPRECATED") == NULL) {
        /* putenv("GUILE_WARN_DEPRECATED=no"); */
        utSetEnvironmentVariable("GUILE_WARN_DEPRECATED", "no");
    }
    gh_enter(argc, argv, myRealMainRoutine);
}
