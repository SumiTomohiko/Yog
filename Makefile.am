bin_PROGRAMS = yog
yog_SOURCES = src/main.c \
	      src/value.c \
	      src/vm.c \
	      src/error.c \
	      src/st.c \
	      src/array.c \
	      src/parser.y \
	      src/lexer.l \
	      src/object.c \
	      src/string.c \
	      src/binary.c \
	      src/compile.c \
	      src/code.c \
	      src/frame.c \
	      src/thread.c
yog_CFLAGS = -Iinclude -Wall
AM_YFLAGS = -d
#AM_LFLAGS = --noline

proto:
	python tools/update_prototype.py

insts:
	python tools/inst.py src/insts.def
