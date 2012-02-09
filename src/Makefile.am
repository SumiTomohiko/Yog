
CORGI_DIR = $(top_srcdir)/corgi
GMP_DIR = $(top_srcdir)/gmp
LIBFFI_DIR = $(top_srcdir)/libffi

LEMON_DIR = $(top_srcdir)/tools/lemon
LEMON = $(LEMON_DIR)/lemon
YACC = $(LEMON)
PYTHON = @PYTHON@
TOOLS_DIR = $(top_srcdir)/tools
YOG = yog

BUILT_SOURCES = $(top_srcdir)/include/yog/token.h keywords.inc
COMMON_SOURCES = arg.c array.c bignum.c binary.c bool.c builtins.c class.c \
		 classmethod.c code.c comparable.c compile.c coroutine.c \
		 dict.c encoding.c error.c eval.c exception.c file.c fixnum.c \
		 float.c frame.c callable.c gc.c get_args.c inst.c lexer.c \
		 main.c misc.c module.c nil.c object.c package.c parser.y \
		 property.c regexp.c repl.c set.c sprintf.c stacktrace.c \
		 string.c symbol.c table.c thread.c value.c vm.c getopt.c \
		 ffi.c env.c handle.c process.c path.c datetime.c dir.c stat.c
DEFAULT_INCLUDES =
CFLAGS_COMMON = -I$(top_srcdir)/include -I$(CORGI_DIR)/include -I$(GMP_DIR) \
		-I$(LIBFFI_DIR)/include -Wall -Werror -g -O2
LDFLAGS_COMMON = @LDFLAGS_COMMON@
LIBS = $(CORGI_DIR)/src/.libs/libcorgi.a $(GMP_DIR)/.libs/libgmp.a \
       $(LIBFFI_DIR)/.libs/libffi.a -lm @LIBS@

EXTRA_PROGRAMS = yog-copying$(EXEEXT) yog-mark-sweep$(EXEEXT) \
		 yog-mark-sweep-compact$(EXEEXT) yog$(EXEEXT)
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
yog_SOURCES = $(COMMON_SOURCES) gc/generational.c gc/copying.c \
	      gc/mark-sweep-compact.c
yog_CFLAGS = $(CFLAGS_COMMON)
yog_LDFLAGS = $(LDFLAGS_COMMON) @LDFLAGS_GENERATIONAL@

INSTS_DEF = insts.def
INST_PY = $(top_srcdir)/tools/inst.py
INST_CMD = make insts

GEN_AM_CMD = $(PYTHON) $(top_srcdir)/tools/update_automake.py .

TEST_CMD = py.test

insts:
	$(PYTHON) $(top_srcdir)/tools/inst.py insts.def ..

am:
	$(GEN_AM_CMD)

builtins.inc: builtins.yog yog2inc.yog
	$(YOG) yog2inc.yog builtins.yog $@

array.inc: array.yog yog2inc.yog
	$(YOG) yog2inc.yog array.yog $@

datetime.inc: datetime.yog yog2inc.yog
	$(YOG) yog2inc.yog datetime.yog $@

object.inc: object.yog yog2inc.yog
	$(YOG) yog2inc.yog object.yog $@

string.inc: string.yog yog2inc.yog
	$(YOG) yog2inc.yog string.yog $@

path.inc: path.yog yog2inc.yog
	$(YOG) yog2inc.yog path.yog $@

set.inc: set.yog yog2inc.yog
	$(YOG) yog2inc.yog set.yog $@

dict.inc: dict.yog yog2inc.yog
	$(YOG) yog2inc.yog dict.yog $@

symbol.inc: symbol.yog yog2inc.yog
	$(YOG) yog2inc.yog symbol.yog $@

callable.inc: callable.yog yog2inc.yog
	$(YOG) yog2inc.yog callable.yog $@

fixnum.inc: fixnum.yog yog2inc.yog
	$(YOG) yog2inc.yog fixnum.yog $@

$(top_srcdir)/include/yog/token.h parser.c: parser.y parser.lt $(LEMON)
	(cd $(LEMON_DIR); $(MAKE))
	$(LEMON) header=$(top_srcdir)/include/yog/token.h parser.y

keywords.inc: keywords
	gperf keywords > $@

eval.inc: $(INSTS_DEF) $(INST_PY)
	$(INST_CMD)

code.inc: code.inc.tmpl $(INSTS_DEF) $(INST_PY)
	$(INST_CMD)

compile.inc: compile.inc.tmpl $(INSTS_DEF) $(INST_PY)
	$(INST_CMD)

inst.c: inst.c.tmpl $(INSTS_DEF) $(INST_PY)
	$(INST_CMD)

$(top_srcdir)/include/yog/inst.h: $(top_srcdir)/include/yog/inst.h.tmpl $(INSTS_DEF) $(INST_PY)
	$(INST_CMD)

$(top_srcdir)/include/yog/opcodes.h: $(top_srcdir)/include/yog/opcodes.h.tmpl $(INSTS_DEF) $(INST_PY)
	$(INST_CMD)

errno.inc: $(TOOLS_DIR)/make_errno.yog $(TOOLS_DIR)/errno
	$(YOG) $(TOOLS_DIR)/make_errno.yog $(TOOLS_DIR)/errno $@

SHIFT_JIS_TABLES = shift_jis2yog_hankaku.inc shift_jis2yog_zenkaku.inc \
		   yog2shift_jis1.inc yog2shift_jis2.inc yog2shift_jis3.inc
$(SHIFT_JIS_TABLES): $(TOOLS_DIR)/make_shift_jis_tables.py sjis-0213-2004-std.txt
	$(PYTHON) $(TOOLS_DIR)/make_shift_jis_tables.py

EUC_JP_TABLES = euc_jp2yog_zenkaku.inc euc_jp2yog_hankaku.inc \
		euc_jp2yog_3bytes.inc yog2euc_jp1.inc yog2euc_jp2.inc \
		yog2euc_jp3.inc
$(EUC_JP_TABLES): $(TOOLS_DIR)/make_euc_jp_tables.py euc-jis-2004-std.txt
	$(PYTHON) $(TOOLS_DIR)/make_euc_jp_tables.py

# vim: tabstop=8 shiftwidth=8 noexpandtab filetype=automake
