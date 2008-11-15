bin_PROGRAMS = yog
yog_SOURCES = src/array.c \
	src/binary.c \
	src/block.c \
	src/bool.c \
	src/builtins.c \
	src/code.c \
	src/compile.c \
	src/error.c \
	src/frame.c \
	src/function.c \
	src/inst.c \
	src/int.c \
	src/klass.c \
	src/lexer.c \
	src/main.c \
	src/method.c \
	src/object.c \
	src/package.c \
	src/parser.y \
	src/string.c \
	src/table.c \
	src/thread.c \
	src/value.c \
	src/vm.c
yog_CFLAGS = -Iinclude -Ionig -Wall
AM_YFLAGS = -d --verbose
#AM_LFLAGS = --noline

SUBDIRS = onig

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

# vim: tabstop=8 shiftwidth=8 noexpandtab filetype=automake
