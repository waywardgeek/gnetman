#ifndef BISON_CIRYTAB_H
# define BISON_CIRYTAB_H

#ifndef YYSTYPE
typedef union {
    utSym stringVal;
    utSym identVal;
    cirAttr attrVal;
    bool boolVal;
} yystype;
# define YYSTYPE yystype
# define YYSTYPE_IS_TRIVIAL 1
#endif
# define        STRING  257
# define        IDENT   258


extern YYSTYPE cirlval;

#endif /* not BISON_CIRYTAB_H */
