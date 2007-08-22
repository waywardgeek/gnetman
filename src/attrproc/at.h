#include "db.h"
#include "atdatabase.h"
#include "atext.h"

/* Lex, yacc stuff */
extern char *atExpressionString;
extern atContext atCurrentContext;
extern double atCurrentValue;
extern uint32 atLineNum;
extern bool atExpressionPassed;
extern int atparse();
extern int atlex();
extern void aterror(char *message, ...);
extern void atwarn(char *message, ...);

