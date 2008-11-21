ONIG_DIR = onig
ONIG_LIB = $(ONIG_DIR)/.libs/libonig.a

SUBDIRS = $(ONIG_DIR)

bin_PROGRAMS = yog
yog_SOURCES = src/array.c \
	src/binary.c \
	src/block.c \
	src/bool.c \
	src/builtins.c \
	src/code.c \
	src/compile.c \
	src/encoding.c \
	src/error.c \
	src/exception.c \
	src/frame.c \
	src/function.c \
	src/inst.c \
	src/int.c \
	src/klass.c \
	src/lexer.c \
	src/main.c \
	src/method.c \
	src/nil.c \
	src/object.c \
	src/package.c \
	src/parser.y \
	src/stacktrace.c \
	src/string.c \
	src/table.c \
	src/thread.c \
	src/value.c \
	src/vm.c
yog_CFLAGS = -Iinclude -I$(ONIG_DIR) -Wall
yog_LDADD = $(ONIG_LIB)

AM_YFLAGS = -d --verbose
#AM_LFLAGS = --noline

INSTS_DEF = src/insts.def
INST_PY = tools/inst.py
INST_CMD = make insts

proto:
	python tools/update_prototype.py

insts:
	python tools/inst.py src/insts.def

am:
	python tools/update_automake.py

test: yog$(EXEEXT)
	py.test

clean-generic:
	find . -name "*.pyc" -print0 | xargs --null rm -f

src/keywords.inc: src/keywords
	gperf $< > $@

src/lexer.c: parser.h

src/thread.inc: $(INSTS_DEF) $(INST_PY)
	$(INST_CMD)

src/code.inc: src/code.inc.tmpl $(INSTS_DEF) $(INST_PY)
	$(INST_CMD)

src/compile.inc: src/compile.inc.tmpl $(INSTS_DEF) $(INST_PY)
	$(INST_CMD)

include/yog/inst.h: include/yog/inst.h.tmpl $(INSTS_DEF) $(INST_PY)
	$(INST_CMD)

# vim: tabstop=8 shiftwidth=8 noexpandtab filetype=automake
