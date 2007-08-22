## $Id$
##

AM_CFLAGS=-DDD_DEBUG

INCLUDES=	-I$(srcdir)/../include
noinst_LIBRARIES=	libcir.a

libcir_a_SOURCES= \
	cir.h \
	cirparse.y \
	cirscan.l \
	cirdatabase.c \
	cirdatabase.h \
	cirread.c \
	cirwrite.c

BUILT_SOURCES=	cirparse.h

AM_LFLAGS=	-f -i -Pcir -o$(LEX_OUTPUT_ROOT).c
AM_YFLAGS=	-d -p cir
