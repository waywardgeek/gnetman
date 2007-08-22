## $Id$
##

AM_CFLAGS= -DDD_DEBUG

noinst_LIBRARIES=libtclwrap.a

libtclwrap_a_SOURCES= \
tclfunc.c \
tclfunc_wrap.c \
tclwrap.c \
tclfunc.h \
tc.h

INCLUDES=@TCL_CFLAGS@ -I$(srcdir)/../include

EXTRA_DIST= tclfunc.i
