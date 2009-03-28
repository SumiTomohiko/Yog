
YOG_DIR = ../..

bin_PROGRAMS = test-mark-sweep-compact

test_mark_sweep_compact_SOURCES = $(YOG_DIR)/src/gc/mark-sweep-compact.c
test_mark_sweep_compact_CFLAGS = -Wall -I$(YOG_DIR)/include \
				 -DTEST_MARK_SWEEP_COMPACT \
				 -DGC_MARK_SWEEP_COMPACT
test_mark_sweep_compact_LDFLAGS = -lcunit
