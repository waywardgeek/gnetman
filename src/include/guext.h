bool guStart(char *rcFileName);
void guStop(void);
/* This is a super-stupid thing for a control tool to do... It makes you call it's main entry
   routine and then it calls your main routing.  In environments where we don't even have a main,
   like in Windows, this will be a problem.  It makes the whole thought of using Guile barely
   tollerable.  Be careful... this routine WILL NEVER RETURN! */
void guEnterGuileMainRoutine(int argc, char **argv,
    void myRealMainRoutine(int argc, char *argv[]));
bool guLoadFile(char *fileName);
