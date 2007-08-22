## $Id$
##

bin_PROGRAMS=gnetman

# This tricks Automake so it defines the $(LINK) variable.
EXTRA_PROGRAMS = fubar
EXTRA_DIST= fubar.c

gnetman_SOURCES=

SUBDIRS=batch spice database tcl include sch hash verilog attrproc util generate pcb lib

# There seems to be a bug in ar.  cirread.o is linked into spice/libcir.a,
# but ar can't find cirReadDesign no matter what I do.  However, linking
# in the cirread.o file manually works!

gnetman_LDADD= \
	batch/libbatch.a \
	spice/libcir.a \
	database/libdatabase.a \
	tcl/libtclwrap.a \
	sch/libsch.a \
	generate/libgenerate.a \
	hash/libhtbl.a \
	util/libutil.a \
	verilog/libverilog.a \
	attrproc/libattrproc.a \
	pcb/libpcb.a \
	spice/cirread.o \
	@TCL_LDFLAGS@ \
	@LEXLIB@

