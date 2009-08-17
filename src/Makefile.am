
ROOT_DIR = ..
ONIG_DIR = $(ROOT_DIR)/onig
BDWGC_DIR = $(ROOT_DIR)/bdwgc
SIGSEGV_DIR = $(ROOT_DIR)/sigsegv
GMP_DIR = $(ROOT_DIR)/gmp

LEMON_DIR = $(ROOT_DIR)/tools/lemon
LEMON = $(LEMON_DIR)/lemon
YACC = $(LEMON)

BUILT_SOURCES = parser.h keywords.inc
COMMON_SOURCES = arg.c \
	array.c \
	bignum.c \
	binary.c \
	block.c \
	bool.c \
	builtins.c \
	classmethod.c \
	code.c \
	compile.c \
	dict.c \
	encoding.c \
	error.c \
	eval.c \
	exception.c \
	file.c \
	fixnum.c \
	float.c \
	frame.c \
	function.c \
	gc.c \
	inst.c \
	klass.c \
	lexer.c \
	main.c \
	module.c \
	nil.c \
	object.c \
	package.c \
	parser.y \
	property.c \
	regexp.c \
	repl.c \
	set.c \
	stacktrace.c \
	string.c \
	symbol.c \
	table.c \
	thread.c \
	value.c \
	vm.c
CFLAGS_COMMON = -I$(ROOT_DIR)/include -I$(ONIG_DIR) -I$(BDWGC_DIR)/include \
		-I$(GMP_DIR) -Wall -DPREFIX=\"$(prefix)\"
LDFLAGS_COMMON = $(ONIG_DIR)/.libs/libonig.a -ldl -lm -export-dynamic \
		 $(GMP_DIR)/.libs/libgmp.a

EXTRA_PROGRAMS = yog-bdw yog-copying yog-mark-sweep yog-mark-swee-compact \
		 yog-mark-sweep-compact yog-generational miniyog
bin_PROGRAMS = @BIN_PROGRAMS@
yog_copying_SOURCES = $(COMMON_SOURCES) gc/copying.c
yog_copying_CFLAGS = $(CFLAGS_COMMON) -DGC_COPYING
yog_copying_LDFLAGS = $(LDFLAGS_COMMON)
yog_mark_sweep_SOURCES = $(COMMON_SOURCES) gc/mark-sweep.c
yog_mark_sweep_CFLAGS = $(CFLAGS_COMMON) -DGC_MARK_SWEEP
yog_mark_sweep_LDFLAGS = $(LDFLAGS_COMMON)
yog_mark_sweep_compact_SOURCES = $(COMMON_SOURCES) gc/mark-sweep-compact.c
yog_mark_sweep_compact_CFLAGS = $(CFLAGS_COMMON) -DGC_MARK_SWEEP_COMPACT
yog_mark_sweep_compact_LDFLAGS = $(LDFLAGS_COMMON)
yog_bdw_SOURCES = $(COMMON_SOURCES) gc/bdw.c
yog_bdw_CFLAGS = $(CFLAGS_COMMON) -DGC_BDW -DGC_PTHREADS
yog_bdw_LDFLAGS = $(LDFLAGS_COMMON) $(BDWGC_DIR)/.libs/libgc.a -lpthread
yog_generational_SOURCES = $(COMMON_SOURCES) gc/generational.c gc/copying.c \
			   gc/mark-sweep-compact.c
yog_generational_CFLAGS = $(CFLAGS_COMMON) -DGC_GENERATIONAL \
			  -I$(SIGSEGV_DIR)/src
yog_generational_LDFLAGS = $(LDFLAGS_COMMON) \
			   $(SIGSEGV_DIR)/src/.libs/libsigsegv.a
miniyog_SOURCES = $(yog_copying_SOURCES)
miniyog_CFLAGS = $(yog_copying_CFLAGS) -DMINIYOG
miniyog_LDFLAGS = $(yog_copying_LDFLAGS)

INSTS_DEF = insts.def
INST_PY = $(ROOT_DIR)/tools/inst.py
INST_CMD = make insts

GEN_AM_CMD = python $(ROOT_DIR)/tools/update_automake.py .

TEST_CMD = py.test

insts:
	python $(ROOT_DIR)/tools/inst.py insts.def ..

am:
	$(GEN_AM_CMD)

builtins.inc: miniyog builtins.yg conv_builtins.yg
	$(MAKE) miniyog
	./miniyog conv_builtins.yg builtins.yg $@

parser.h parser.c: parser.y parser.lt $(LEMON)
	(cd $(LEMON_DIR); $(MAKE))
	$(LEMON) parser.y

keywords.inc: keywords
	gperf $< > $@

#keywords lexer.c: parser.h

eval.inc: $(INSTS_DEF) $(INST_PY)
	$(INST_CMD)

code.inc: code.inc.tmpl $(INSTS_DEF) $(INST_PY)
	$(INST_CMD)

compile.inc: compile.inc.tmpl $(INSTS_DEF) $(INST_PY)
	$(INST_CMD)

$(ROOT_DIR)/include/yog/inst.h: $(ROOT_DIR)/include/yog/inst.h.tmpl $(INSTS_DEF) $(INST_PY)
	$(INST_CMD)

VERSION_H = $(ROOT_DIR)/include/yog/version.h
VERSION_H_M4 = $(VERSION_H).m4
VERSION_H_NEW = $(VERSION_H).new

update-version:
	if [ -d "$(ROOT_DIR)/.hg" ]; then \
		m4 -DREVISION="`hg parent --template='{rev} {node|short}'`" $(VERSION_H_M4) > $(VERSION_H_NEW); \
		if [ -f $(VERSION_H) ]; then \
			cmp $(VERSION_H) $(VERSION_H_NEW); \
			if [ $$? != 0 ]; then \
				mv -f $(VERSION_H_NEW) $(VERSION_H); \
			fi \
		else \
			mv -f $(VERSION_H_NEW) $(VERSION_H); \
		fi \
	fi

Makefile.am: Makefile.am.tmpl
	$(GEN_AM_CMD)

# vim: tabstop=8 shiftwidth=8 noexpandtab filetype=automake
