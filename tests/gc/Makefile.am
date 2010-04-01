
DEFAULT_INCLUDES =
SRC_DIR = $(top_srcdir)/src
LIBS = $(top_srcdir)/onig/.libs/libonig.a $(top_srcdir)/gmp/.libs/libgmp.a -lm \
       -lpthread -ldl

bin_PROGRAMS = test-mark-sweep-compact$(EXEEXT)
test_mark_sweep_compact_SOURCES = $(SRC_DIR)/gc/mark-sweep-compact.c \
				  $(SRC_DIR)/gc.c $(SRC_DIR)/error.c \
				  $(SRC_DIR)/vm.c $(SRC_DIR)/table.c \
				  $(SRC_DIR)/array.c $(SRC_DIR)/dict.c \
				  $(SRC_DIR)/string.c $(SRC_DIR)/get_args.c \
				  $(SRC_DIR)/encoding.c $(SRC_DIR)/callable.c \
				  $(SRC_DIR)/regexp.c $(SRC_DIR)/object.c \
				  $(SRC_DIR)/misc.c $(SRC_DIR)/eval.c \
				  $(SRC_DIR)/compile.c $(SRC_DIR)/parser.c \
				  $(SRC_DIR)/lexer.c $(SRC_DIR)/binary.c \
				  $(SRC_DIR)/code.c $(SRC_DIR)/class.c \
				  $(SRC_DIR)/float.c $(SRC_DIR)/bignum.c \
				  $(SRC_DIR)/fixnum.c $(SRC_DIR)/value.c \
				  $(SRC_DIR)/classmethod.c $(SRC_DIR)/frame.c \
				  $(SRC_DIR)/sprintf.c $(SRC_DIR)/property.c \
				  $(SRC_DIR)/inst.c $(SRC_DIR)/arg.c \
				  $(SRC_DIR)/set.c $(SRC_DIR)/module.c \
				  $(SRC_DIR)/package.c $(SRC_DIR)/exception.c \
				  $(SRC_DIR)/stacktrace.c $(SRC_DIR)/thread.c \
				  $(SRC_DIR)/bool.c $(SRC_DIR)/coroutine.c \
				  $(SRC_DIR)/builtins.c $(SRC_DIR)/symbol.c \
				  $(SRC_DIR)/nil.c $(SRC_DIR)/file.c \
				  $(SRC_DIR)/comparable.c
test_mark_sweep_compact_CFLAGS = -DTEST -DGC_MARK_SWEEP_COMPACT
test_mark_sweep_compact_LDFLAGS = -lcunit

# vim: tabstop=8 shiftwidth=8 noexpandtab
