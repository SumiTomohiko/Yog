
ROOT_DIR = ..
ONIG_DIR = $(ROOT_DIR)/onig
BDWGC_DIR = $(ROOT_DIR)/bdwgc

LEMON_DIR = $(ROOT_DIR)/tools/lemon
LEMON = $(LEMON_DIR)/lemon
YACC = $(LEMON)

BUILT_SOURCES = parser.h keywords.inc

lib_LTLIBRARIES = libyog-copying.la \
		  libyog-mark-sweep.la \
		  libyog-mark-sweep-compact.la \
		  libyog-bdw.la \
		  libyog-generational.la
COMMON_SOURCES = arg.c \
	array.c \
	binary.c \
	block.c \
	bool.c \
	builtins.c \
	code.c \
	compile.c \
	encoding.c \
	error.c \
	exception.c \
	frame.c \
	function.c \
	inst.c \
	int.c \
	klass.c \
	lexer.c \
	main.c \
	method.c \
	nil.c \
	object.c \
	package.c \
	parser.y \
	regexp.c \
	stacktrace.c \
	string.c \
	table.c \
	thread.c \
	value.c \
	vm.c
COMMON_CFLAGS = -I$(ROOT_DIR)/include -I$(ONIG_DIR) -I$(BDWGC_DIR)/include -Wall
libyog_copying_la_SOURCES = $(COMMON_SOURCES) gc/copying.c
libyog_copying_la_CFLAGS = $(COMMON_CFLAGS) -DGC_COPYING
libyog_mark_sweep_la_SOURCES = $(COMMON_SOURCES) gc/mark-sweep.c
libyog_mark_sweep_la_CFLAGS = $(COMMON_CFLAGS) -DGC_MARK_SWEEP
libyog_mark_sweep_compact_la_SOURCES = $(COMMON_SOURCES) gc/mark-sweep-compact.c
libyog_mark_sweep_compact_la_CFLAGS = $(COMMON_CFLAGS) -DGC_MARK_SWEEP_COMPACT
libyog_bdw_la_SOURCES = $(COMMON_SOURCES) gc/bdw.c
libyog_bdw_la_CFLAGS = $(COMMON_CFLAGS) -DGC_BDW
libyog_generational_la_SOURCES = $(COMMON_SOURCES) gc/generational.c \
				 gc/copying.c gc/mark-sweep-compact.c
libyog_generational_la_CFLAGS = $(COMMON_CFLAGS) -DGC_GENERATIONAL

bin_PROGRAMS = yog-copying \
	       yog-mark-sweep \
	       yog-mark-sweep-compact \
	       yog-bdw \
	       yog-generational
yog_COMMON_LDFLAGS = $(ONIG_DIR)/.libs/libonig.a
yog_copying_SOURCES = $(libyog_copying_la_SOURCES)
yog_copying_CFLAGS = $(libyog_copying_la_CFLAGS)
yog_copying_LDFLAGS = $(yog_COMMON_LDFLAGS)
yog_mark_sweep_SOURCES = $(libyog_mark_sweep_la_SOURCES)
yog_mark_sweep_CFLAGS = $(libyog_mark_sweep_la_CFLAGS)
yog_mark_sweep_LDFLAGS = $(yog_COMMON_LDFLAGS)
yog_mark_sweep_compact_SOURCES = $(libyog_mark_sweep_compact_la_SOURCES)
yog_mark_sweep_compact_CFLAGS = $(libyog_mark_sweep_compact_la_CFLAGS)
yog_mark_sweep_compact_LDFLAGS = $(yog_COMMON_LDFLAGS)
yog_bdw_SOURCES = $(libyog_bdw_la_SOURCES)
yog_bdw_CFLAGS = $(libyog_bdw_la_CFLAGS)
yog_bdw_LDFLAGS = $(yog_COMMON_LDFLAGS) $(BDWGC_DIR)/.libs/libgc.a -lpthread
yog_generational_SOURCES = $(libyog_generational_la_SOURCES)
yog_generational_CFLAGS = $(libyog_generational_la_CFLAGS)
yog_generational_LDFLAGS = $(yog_COMMON_LDFLAGS)

INSTS_DEF = insts.def
INST_PY = $(ROOT_DIR)/tools/inst.py
INST_CMD = make insts

GEN_AM_CMD = python $(ROOT_DIR)/tools/update_automake.py .

TEST_CMD = py.test

insts:
	python $(ROOT_DIR)/tools/inst.py insts.def ..

am:
	$(GEN_AM_CMD)

parser.c: parser.y parser.lt $(LEMON)
	(cd $(LEMON_DIR); make)
	$(LEMON) parser.y

keywords.inc: keywords
	gperf $< > $@

#keywords lexer.c: parser.h

thread.inc: $(INSTS_DEF) $(INST_PY)
	$(INST_CMD)

code.inc: code.inc.tmpl $(INSTS_DEF) $(INST_PY)
	$(INST_CMD)

compile.inc: compile.inc.tmpl $(INSTS_DEF) $(INST_PY)
	$(INST_CMD)

$(ROOT_DIR)/include/yog/inst.h: $(ROOT_DIR)/include/yog/inst.h.tmpl $(INSTS_DEF) $(INST_PY)
	$(INST_CMD)

Makefile.am: Makefile.am.tmpl
	$(GEN_AM_CMD)

# vim: tabstop=8 shiftwidth=8 noexpandtab filetype=automake
