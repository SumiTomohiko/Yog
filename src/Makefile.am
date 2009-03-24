
ROOT_DIR = ..
ONIG_DIR = $(ROOT_DIR)/onig
BDWGC_DIR = $(ROOT_DIR)/bdwgc

LEMON_DIR = $(ROOT_DIR)/tools/lemon
LEMON = $(LEMON_DIR)/lemon
YACC = $(LEMON)

BUILT_SOURCES = parser.h keywords.inc

lib_LTLIBRARIES = libyog.la
libyog_la_SOURCES = arg.c \
	array.c \
	binary.c \
	block.c \
	bool.c \
	builtins.c \
	code.c \
	compile.c \
	encoding.c \
	error.c \
	exception.c \
	frame.c \
	function.c \
	gc/copying.c \
	gc/mark-sweep-compact.c \
	gc/mark-sweep.c \
	inst.c \
	int.c \
	klass.c \
	lexer.c \
	main.c \
	method.c \
	nil.c \
	object.c \
	package.c \
	parser.y \
	regexp.c \
	stacktrace.c \
	string.c \
	table.c \
	thread.c \
	token.c \
	value.c \
	vm.c
libyog_la_CFLAGS = -I$(ROOT_DIR)/include -I$(ONIG_DIR) -I$(BDWGC_DIR)/include -Wall

INSTS_DEF = insts.def
INST_PY = $(ROOT_DIR)/tools/inst.py
INST_CMD = make insts

GEN_AM_CMD = python $(ROOT_DIR)/tools/update_automake.py .

TEST_CMD = py.test

insts:
	python $(ROOT_DIR)/tools/inst.py insts.def ..

am:
	$(GEN_AM_CMD)

parser.c: parser.y parser.lt $(LEMON)
	(cd $(LEMON_DIR); make)
	$(LEMON) parser.y

keywords.inc: keywords
	gperf $< > $@

#keywords lexer.c: parser.h

thread.inc: $(INSTS_DEF) $(INST_PY)
	$(INST_CMD)

code.inc: code.inc.tmpl $(INSTS_DEF) $(INST_PY)
	$(INST_CMD)

compile.inc: compile.inc.tmpl $(INSTS_DEF) $(INST_PY)
	$(INST_CMD)

$(ROOT_DIR)/include/yog/inst.h: $(ROOT_DIR)/include/yog/inst.h.tmpl $(INSTS_DEF) $(INST_PY)
	$(INST_CMD)

Makefile.am: Makefile.am.tmpl
	$(GEN_AM_CMD)

# vim: tabstop=8 shiftwidth=8 noexpandtab filetype=automake
