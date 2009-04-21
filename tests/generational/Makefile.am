
YOG_DIR = ../..
SRC_DIR = $(YOG_DIR)/src
GC_DIR = $(SRC_DIR)/gc
SIGSEGV_DIR = $(YOG_DIR)/sigsegv

bin_PROGRAMS = test-generational

test_generational_SOURCES = $(GC_DIR)/generational.c $(GC_DIR)/copying.c \
			    $(GC_DIR)/mark-sweep-compact.c $(SRC_DIR)/value.c \
			    $(SRC_DIR)/error.c $(SRC_DIR)/object.c \
			    $(SRC_DIR)/table.c $(SRC_DIR)/vm.c \
			    $(SRC_DIR)/klass.c $(SRC_DIR)/string.c \
			    $(SRC_DIR)/thread.c $(SRC_DIR)/encoding.c \
			    $(SRC_DIR)/frame.c $(SRC_DIR)/array.c \
			    $(SRC_DIR)/binary.c $(SRC_DIR)/package.c \
			    $(SRC_DIR)/method.c $(SRC_DIR)/function.c \
			    $(SRC_DIR)/nil.c $(SRC_DIR)/exception.c \
			    $(SRC_DIR)/arg.c $(SRC_DIR)/stacktrace.c \
			    $(SRC_DIR)/int.c $(SRC_DIR)/regexp.c \
			    $(SRC_DIR)/block.c $(SRC_DIR)/bool.c \
			    $(SRC_DIR)/builtins.c $(SRC_DIR)/eval.c \
			    $(SRC_DIR)/float.c
test_generational_CFLAGS = -Wall -I$(YOG_DIR)/include -I$(SIGSEGV_DIR)/src \
			   -DTEST_GENERATIONAL -DGC_GENERATIONAL
test_generational_LDFLAGS = -lcunit -lonig $(SIGSEGV_DIR)/src/.libs/libsigsegv.a
