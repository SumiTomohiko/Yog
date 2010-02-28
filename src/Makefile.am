
ROOT_DIR = ..
ONIG_DIR = $(ROOT_DIR)/onig
BDWGC_DIR = $(ROOT_DIR)/bdwgc
GMP_DIR = $(ROOT_DIR)/gmp

LEMON_DIR = $(ROOT_DIR)/tools/lemon
LEMON = $(LEMON_DIR)/lemon
YACC = $(LEMON)
PYTHON = @PYTHON@
MINIYOG = miniyog$(EXEEXT)
TOOLS_DIR = $(ROOT_DIR)/tools

BUILT_SOURCES = $(ROOT_DIR)/include/yog/token.h keywords.inc
COMMON_SOURCES = arg.c array.c bignum.c binary.c bool.c builtins.c class.c \
		 classmethod.c code.c comparable.c compile.c coroutine.c \
		 dict.c encoding.c error.c eval.c exception.c file.c fixnum.c \
		 float.c frame.c callable.c gc.c get_args.c inst.c lexer.c \
		 main.c misc.c module.c nil.c object.c package.c parser.y \
		 property.c regexp.c repl.c set.c sprintf.c stacktrace.c \
		 string.c symbol.c table.c thread.c value.c vm.c getopt.c
DEFAULT_INCLUDES = -I.
CFLAGS_COMMON = -I$(ROOT_DIR)/include -I$(ONIG_DIR) -I$(BDWGC_DIR)/include \
		-I$(GMP_DIR) -Wall -DPREFIX=\"$(prefix)\" @CFLAGS_COMMON@
LDFLAGS_COMMON = @LDFLAGS_COMMON@
LIBS = $(ONIG_DIR)/.libs/libonig.a $(GMP_DIR)/.libs/libgmp.a -lm @LIBS@

EXTRA_PROGRAMS = yog-bdw$(EXEEXT) yog-copying$(EXEEXT) yog-mark-sweep$(EXEEXT) \
		 yog-mark-sweep-compact$(EXEEXT) yog-generational$(EXEEXT) \
		 miniyog$(EXEEXT)
bin_PROGRAMS = @BIN_PROGRAMS@
yog_copying_SOURCES = $(COMMON_SOURCES) gc/copying.c
yog_copying_CFLAGS = $(CFLAGS_COMMON) -DGC_COPYING
yog_copying_LDFLAGS = $(LDFLAGS_COMMON) @LDFLAGS_COPYING@
yog_mark_sweep_SOURCES = $(COMMON_SOURCES) gc/mark-sweep.c
yog_mark_sweep_CFLAGS = $(CFLAGS_COMMON) -DGC_MARK_SWEEP
yog_mark_sweep_LDFLAGS = $(LDFLAGS_COMMON) @LDFLAGS_MARK_SWEEP@
yog_mark_sweep_compact_SOURCES = $(COMMON_SOURCES) gc/mark-sweep-compact.c
yog_mark_sweep_compact_CFLAGS = $(CFLAGS_COMMON) -DGC_MARK_SWEEP_COMPACT
yog_mark_sweep_compact_LDFLAGS = $(LDFLAGS_COMMON) @LDFLAGS_MARK_SWEEP_COMPACT@
yog_bdw_SOURCES = $(COMMON_SOURCES) gc/bdw.c
yog_bdw_CFLAGS = $(CFLAGS_COMMON) -DGC_BDW -DGC_PTHREADS
yog_bdw_LDFLAGS = $(LDFLAGS_COMMON) @LDFLAGS_BDW@
yog_bdw_LDADD = $(BDWGC_DIR)/.libs/libgc.a
yog_generational_SOURCES = $(COMMON_SOURCES) gc/generational.c gc/copying.c \
			   gc/mark-sweep-compact.c
yog_generational_CFLAGS = $(CFLAGS_COMMON) -DGC_GENERATIONAL
yog_generational_LDFLAGS = $(LDFLAGS_COMMON) @LDFLAGS_GENERATIONAL@
miniyog_SOURCES = $(yog_copying_SOURCES)
miniyog_CFLAGS = $(yog_copying_CFLAGS) -DMINIYOG
miniyog_LDFLAGS = $(LDFLAGS_COMMON) @LDFLAGS_MINIYOG@

INSTS_DEF = insts.def
INST_PY = $(ROOT_DIR)/tools/inst.py
INST_CMD = make insts

GEN_AM_CMD = $(PYTHON) $(ROOT_DIR)/tools/update_automake.py .

TEST_CMD = py.test

insts:
	$(PYTHON) $(ROOT_DIR)/tools/inst.py insts.def ..

am:
	$(GEN_AM_CMD)

builtins.inc: $(MINIYOG) builtins.yg yg2inc.yg
	$(MAKE) $(MINIYOG)
	./$(MINIYOG) yg2inc.yg builtins.yg $@

array.inc: $(MINIYOG) array.yg yg2inc.yg
	$(MAKE) $(MINIYOG)
	./$(MINIYOG) yg2inc.yg array.yg $@

object.inc: $(MINIYOG) object.yg yg2inc.yg
	$(MAKE) $(MINIYOG)
	./$(MINIYOG) yg2inc.yg object.yg $@

string.inc: $(MINIYOG) string.yg yg2inc.yg
	$(MAKE) $(MINIYOG)
	./$(MINIYOG) yg2inc.yg string.yg $@

dict.inc: $(MINIYOG) dict.yg yg2inc.yg
	$(MAKE) $(MINIYOG)
	./$(MINIYOG) yg2inc.yg dict.yg $@

$(ROOT_DIR)/include/yog/token.h parser.c: parser.y parser.lt $(LEMON)
	(cd $(LEMON_DIR); $(MAKE))
	$(LEMON) header=$(ROOT_DIR)/include/yog/token.h parser.y

keywords.inc: keywords
	gperf $< > $@

eval.inc: $(INSTS_DEF) $(INST_PY)
	$(INST_CMD)

code.inc: code.inc.tmpl $(INSTS_DEF) $(INST_PY)
	$(INST_CMD)

compile.inc: compile.inc.tmpl $(INSTS_DEF) $(INST_PY)
	$(INST_CMD)

inst.c: inst.c.tmpl $(INSTS_DEF) $(INST_PY)
	$(INST_CMD)

$(ROOT_DIR)/include/yog/inst.h: $(ROOT_DIR)/include/yog/inst.h.tmpl $(INSTS_DEF) $(INST_PY)
	$(INST_CMD)

$(ROOT_DIR)/include/yog/opcodes.h: $(ROOT_DIR)/include/yog/opcodes.h.tmpl $(INSTS_DEF) $(INST_PY)
	$(INST_CMD)

errno.inc: $(TOOLS_DIR)/make_errno.yg $(TOOLS_DIR)/errno
	$(MINIYOG) $(TOOLS_DIR)/make_errno.yg $(TOOLS_DIR)/errno $@

# vim: tabstop=8 shiftwidth=8 noexpandtab filetype=automake
