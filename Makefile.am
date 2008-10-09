bin_PROGRAMS = yog
yog_SOURCES = src/array.c \
	src/binary.c \
	src/code.c \
	src/compile.c \
	src/error.c \
	src/frame.c \
	src/function.c \
	src/int.c \
	src/klass.c \
	src/lexer.l \
	src/main.c \
	src/object.c \
	src/parser.y \
	src/st.c \
	src/string.c \
	src/thread.c \
	src/value.c \
	src/vm.c
yog_CFLAGS = -Iinclude -Wall
AM_YFLAGS = -d --verbose
#AM_LFLAGS = --noline

proto:
	python tools/update_prototype.py

insts:
	python tools/inst.py src/insts.def

am:
	python tools/update_automake.py

# vim: tabstop=8 shiftwidth=8 noexpandtab filetype=automake
