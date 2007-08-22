dnl $Id$
dnl Process this file with autoconf to produce a configure script.

AC_INIT(src/batch/bamain.c)
AM_INIT_AUTOMAKE(gnetman, `date +%d%b%g`)
AM_CONFIG_HEADER(config.h)

ac_default_prefix='${HOME}/geda'

dnl Checks for programs.
AC_PROG_CC
AC_PROG_CC_STDC
AM_PROG_LEX
AC_PROG_INSTALL
AC_PROG_RANLIB
AC_PROG_YACC

dnl This hack is required on Ubuntu...  wish I could find another way!
TCL_CONFIG_SH="/usr/lib/tcl8.4/tclConfig.sh"

AC_PATH_PROG( TCL_CONFIG_SH, tclConfig.sh, AC_ERROR([You must have tcl installed]),
	/usr/lib /usr/local/lib /usr/pkg/lib /opt/tcl/lib )
AC_MSG_CHECKING([for tcl libraries])
. $TCL_CONFIG_SH
TCL_LDFLAGS="$TCL_LIB_SPEC $TCL_LIBS"
AC_MSG_RESULT([$TCL_LDFLAGS])
AC_SUBST(TCL_LDFLAGS)

AC_MSG_CHECKING([for tcl include flags])
TCL_CFLAGS="$TCL_INCLUDE_SPEC"
AC_SUBST(TCL_CFLAGS)
AC_MSG_RESULT([$TCL_CFLAGS])

dnl Checks for libraries.

dnl Checks for header files.
dnl Checks for typedefs, structures, and compiler characteristics.
dnl Checks for data types
dnl Checks for library functions.

AC_OUTPUT(
	Makefile
	sch/Makefile
	src/Makefile
	src/batch/Makefile
	src/spice/Makefile
	src/database/Makefile
	src/tcl/Makefile
	src/include/Makefile
	src/sch/Makefile
	src/util/Makefile
	src/verilog/Makefile
	src/attrproc/Makefile
	src/hash/Makefile
	src/generate/Makefile
	src/pcb/Makefile
	src/lib/Makefile
	sym/Makefile
	test/Makefile
	doc/Makefile
	)

