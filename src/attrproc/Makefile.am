## $Id$
##

AM_CFLAGS=-DDD_DEBUG

INCLUDES=	-I$(srcdir)/../include
noinst_LIBRARIES=	libattrproc.a

libattrproc_a_SOURCES= \
	at.h \
	attrproc.c \
	atparse.y \
	atscan.l \
	atdatabase.c \
	atdatabase.h

BUILT_SOURCES=	atparse.h

AM_LFLAGS=	-f -i -Pat -o$(LEX_OUTPUT_ROOT).c
AM_YFLAGS=	-d -p at
