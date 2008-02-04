#include <tcl.h>
#include "tc.h"

char *config_dir;

extern int Tclfunc_SafeInit(Tcl_Interp *interp);
extern int Tclfunc_Init(Tcl_Interp *interp);

static Tcl_Interp *interp = NULL;


void tcStart(void)
{
   interp = Tcl_CreateInterp();

   config_dir = utGetConfigDirectory();
   Tcl_Init(interp);
   /*Tclfunc_SafeInit(interp);*/
   Tclfunc_Init(interp);
}

void tcStop(void)
{
   if(interp) {
       Tcl_DeleteInterp(interp);
   }
   else {
      utWarning("Internal Tcl interpreter error while stopping it");
   }
}


/*--------------------------------------------------------------------------------------------------
  Run a Tcl script, and return the result.
--------------------------------------------------------------------------------------------------*/
bool tcRunScript(
   const char *fileName)
{
   bool result;

   if(!interp) {
      utWarning("Tcl interpreter initialisation error while trying to run a script");
      return 0;
   }

   result = Tcl_EvalFile(interp, (char *)fileName) == TCL_OK;
   if (!result) {
      utWarning("Tcl error in file %s, line %d: %s", fileName,
         interp->errorLine, interp->result);
   }
   return result;
}

/*--------------------------------------------------------------------------------------------------
  Run a Tcl interpreter.
--------------------------------------------------------------------------------------------------*/
bool tcRunInterpreter(void)
{
   int result, xChar;
   char commandLine[UTSTRLEN], c;

   if(!interp) {
      utWarning("Tcl interpreter initialisation error while trying to behave interactive");
      return 0;
   }

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
   return true;
}
