#include <tcl.h>
#include "tc.h"

char *config_dir;
extern int Tclfunc_SafeInit(Tcl_Interp *interp);
extern int Tclfunc_Init(Tcl_Interp *interp);

/*--------------------------------------------------------------------------------------------------
  Run a Tcl script, and return the result.
--------------------------------------------------------------------------------------------------*/
bool tcRunScript(
   const char *fileName)
{
   Tcl_Interp *interp = Tcl_CreateInterp();
   bool result;

   config_dir = utGetConfigDirectory();
   Tcl_Init(interp);
   /*Tclfunc_SafeInit(interp);*/
   Tclfunc_Init(interp);
   result = Tcl_EvalFile(interp, (char *)fileName) == TCL_OK;
   if (!result) {
      utWarning("Tcl error in file %s, line %d: %s", fileName,
         interp->errorLine, interp->result);
   }
   Tcl_DeleteInterp(interp);
   return result;
}

/*--------------------------------------------------------------------------------------------------
  Run a Tcl interpreter.
--------------------------------------------------------------------------------------------------*/
bool tcRunInterpreter(void)
{
   Tcl_Interp *interp = Tcl_CreateInterp();
   int result, xChar;
   char commandLine[UTSTRLEN], c;

   config_dir = utGetConfigDirectory();
   Tcl_Init(interp);
   /*Tclfunc_SafeInit(interp);*/
   Tclfunc_Init(interp);
   do {
      printf("%s", "\n% ");
      xChar = 0;
      c = getchar();
      while (xChar + 1 < UTSTRLEN && c != '\n') {
              commandLine[xChar++] = c;
        c = getchar();
      }
      commandLine[xChar] = '\0';
      result = Tcl_Eval(interp, commandLine);
      if (result == TCL_ERROR) {
        utWarning("Tcl error: %s", interp->result);
      }
   } while (result != TCL_RETURN);
   Tcl_DeleteInterp(interp);
   return true;
}
