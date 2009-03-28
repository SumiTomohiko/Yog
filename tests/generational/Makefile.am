
YOG_DIR = ../..
GC_DIR = $(YOG_DIR)/src/gc

bin_PROGRAMS = test-generational

test_generational_SOURCES = $(GC_DIR)/generational.c $(GC_DIR)/copying.c \
			    $(GC_DIR)/mark-sweep-compact.c
test_generational_CFLAGS = -Wall -I$(YOG_DIR)/include -DTEST_GENERATIONAL \
			   -DGC_GENERATIONAL
test_generational_LDFLAGS = -lcunit
