## $Id$
##

AM_CFLAGS=-Wall -W -Wno-unused-parameter -Wno-unused-function -DDD_DEBUG

INCLUDES=	-I$(srcdir)/../include
noinst_LIBRARIES=	libverilog.a

libverilog_a_SOURCES= \
	vr.h \
	vrwrite.c \
	vrread.c \
	vrparse.y \
	vrscan.l \
	vrdatabase.c \
	vrdatabase.h

BUILT_SOURCES=	vrparse.h

AM_LFLAGS=	-f -Pvr -o$(LEX_OUTPUT_ROOT).c
AM_YFLAGS=	-d -p vr
