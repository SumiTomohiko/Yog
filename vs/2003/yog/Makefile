
ROOT_DIR = ..\..\..
SRC_DIR = $(ROOT_DIR)\src
TOOLS_DIR = $(ROOT_DIR)\tools
LEMON_DIR = $(TOOLS_DIR)\lemon
INCLUDE_DIR = $(ROOT_DIR)\include

MINIYOG = miniyog.exe
YOG = yog.exe

COMMON_CFLAGS = $(CFLAGS) /c /I. /Iinclude /I$(INCLUDE_DIR) /I$(ROOT_DIR)\src \
		/I$(ROOT_DIR)\onig /I..\gmp /I..\pthreads-w32 /nologo /O2 \
		/QI0f /Wall /Z7 /TP /DPTW32_STATIC_LIB /DONIG_EXTERN=extern \
		/MT /D_WIN32_WINNT=0x0400
MINIYOG_CFLAGS = $(COMMON_CFLAGS) /DMINIYOG /DGC_COPYING
YOG_CFLAGS = $(COMMON_CFLAGS) /DGC_COPYING

LINK = link.exe
LDFLAGS = $(LDFLAGS) /DEBUG /NOLOGO /SUBSYSTEM:CONSOLE /INCREMENTAL:no
LIBS = ..\gmp\gmp.lib $(ROOT_DIR)\onig\onig_s.lib \
       ..\pthreads-w32\pthreadVC2d.lib ws2_32.lib

PYTHON = python

YOG_OBJS = $(SRC_DIR)\yog-arg.obj $(SRC_DIR)\yog-array.obj \
	   $(SRC_DIR)\yog-bignum.obj $(SRC_DIR)\yog-binary.obj \
	   $(SRC_DIR)\yog-bool.obj $(SRC_DIR)\yog-coroutine.obj \
	   $(SRC_DIR)\yog-builtins.obj $(SRC_DIR)\yog-class.obj \
	   $(SRC_DIR)\yog-classmethod.obj $(SRC_DIR)\yog-code.obj \
	   $(SRC_DIR)\yog-comparable.obj $(SRC_DIR)\yog-compile.obj \
	   $(SRC_DIR)\yog-dict.obj $(SRC_DIR)\yog-encoding.obj \
	   $(SRC_DIR)\yog-error.obj $(SRC_DIR)\yog-eval.obj \
	   $(SRC_DIR)\yog-exception.obj $(SRC_DIR)\yog-file.obj \
	   $(SRC_DIR)\yog-fixnum.obj $(SRC_DIR)\yog-float.obj \
	   $(SRC_DIR)\yog-frame.obj $(SRC_DIR)\yog-callable.obj \
	   $(SRC_DIR)\yog-gc.obj $(SRC_DIR)\yog-inst.obj \
	   $(SRC_DIR)\yog-lexer.obj $(SRC_DIR)\yog-main.obj \
	   $(SRC_DIR)\yog-misc.obj $(SRC_DIR)\yog-module.obj \
	   $(SRC_DIR)\yog-nil.obj $(SRC_DIR)\yog-object.obj \
	   $(SRC_DIR)\yog-package.obj $(SRC_DIR)\yog-parser.obj \
	   $(SRC_DIR)\yog-property.obj $(SRC_DIR)\yog-regexp.obj \
	   $(SRC_DIR)\yog-repl.obj $(SRC_DIR)\yog-set.obj \
	   $(SRC_DIR)\yog-stacktrace.obj $(SRC_DIR)\yog-string.obj \
	   $(SRC_DIR)\yog-symbol.obj $(SRC_DIR)\yog-table.obj \
	   $(SRC_DIR)\yog-thread.obj $(SRC_DIR)\yog-value.obj \
	   $(SRC_DIR)\yog-vm.obj $(SRC_DIR)\gc\yog-copying.obj \
	   $(SRC_DIR)\yog-get_args.obj $(SRC_DIR)\yog-sprintf.obj icon.RES \
	   $(SRC_DIR)\yog-getopt.obj

MINIYOG_OBJS = $(SRC_DIR)\miniyog-arg.obj $(SRC_DIR)\miniyog-array.obj \
	       $(SRC_DIR)\miniyog-bignum.obj $(SRC_DIR)\miniyog-binary.obj \
	       $(SRC_DIR)\miniyog-bool.obj $(SRC_DIR)\miniyog-coroutine.obj \
	       $(SRC_DIR)\miniyog-builtins.obj $(SRC_DIR)\miniyog-class.obj \
	       $(SRC_DIR)\miniyog-classmethod.obj $(SRC_DIR)\miniyog-code.obj \
	       $(SRC_DIR)\miniyog-comparable.obj \
	       $(SRC_DIR)\miniyog-compile.obj $(SRC_DIR)\miniyog-dict.obj \
	       $(SRC_DIR)\miniyog-encoding.obj $(SRC_DIR)\miniyog-error.obj \
	       $(SRC_DIR)\miniyog-eval.obj $(SRC_DIR)\miniyog-exception.obj \
	       $(SRC_DIR)\miniyog-file.obj $(SRC_DIR)\miniyog-fixnum.obj \
	       $(SRC_DIR)\miniyog-float.obj $(SRC_DIR)\miniyog-frame.obj \
	       $(SRC_DIR)\miniyog-callable.obj $(SRC_DIR)\miniyog-gc.obj \
	       $(SRC_DIR)\miniyog-inst.obj $(SRC_DIR)\miniyog-lexer.obj \
	       $(SRC_DIR)\miniyog-main.obj $(SRC_DIR)\miniyog-misc.obj \
	       $(SRC_DIR)\miniyog-module.obj $(SRC_DIR)\miniyog-nil.obj \
	       $(SRC_DIR)\miniyog-object.obj $(SRC_DIR)\miniyog-package.obj \
	       $(SRC_DIR)\miniyog-parser.obj $(SRC_DIR)\miniyog-property.obj \
	       $(SRC_DIR)\miniyog-regexp.obj $(SRC_DIR)\miniyog-repl.obj \
	       $(SRC_DIR)\miniyog-set.obj $(SRC_DIR)\miniyog-stacktrace.obj \
	       $(SRC_DIR)\miniyog-string.obj $(SRC_DIR)\miniyog-symbol.obj \
	       $(SRC_DIR)\miniyog-table.obj $(SRC_DIR)\miniyog-thread.obj \
	       $(SRC_DIR)\miniyog-value.obj $(SRC_DIR)\miniyog-vm.obj \
	       $(SRC_DIR)\gc\miniyog-copying.obj \
	       $(SRC_DIR)\miniyog-get_args.obj $(SRC_DIR)\miniyog-sprintf.obj \
	       icon.RES $(SRC_DIR)\miniyog-getopt.obj

all: $(YOG)

$(YOG): $(YOG_OBJS)
	$(LINK) $(LDFLAGS) /OUT:$@ $(LIBS) $(YOG_OBJS)

$(SRC_DIR)\yog-arg.obj: $(SRC_DIR)\arg.c
	$(CC) $(YOG_CFLAGS) /Fo$@ $?

$(SRC_DIR)\yog-array.obj: $(SRC_DIR)\array.c $(SRC_DIR)\array.inc
	$(CC) $(YOG_CFLAGS) /Fo$@ $(SRC_DIR)\array.c

$(SRC_DIR)\array.inc: $(SRC_DIR)\yg2inc.yg $(SRC_DIR)\array.yg
	$(MAKE) /$(MAKEFLAGS) /NOLOGO $(MINIYOG)
	$(MINIYOG) $(SRC_DIR)\yg2inc.yg $(SRC_DIR)\array.yg $@

$(SRC_DIR)\yog-bignum.obj: $(SRC_DIR)\bignum.c
	$(CC) $(YOG_CFLAGS) /Fo$@ $?

$(SRC_DIR)\yog-binary.obj: $(SRC_DIR)\binary.c
	$(CC) $(YOG_CFLAGS) /Fo$@ $?

$(SRC_DIR)\yog-bool.obj: $(SRC_DIR)\bool.c
	$(CC) $(YOG_CFLAGS) /Fo$@ $?

$(SRC_DIR)\yog-builtins.obj: $(MINIYOG) $(SRC_DIR)\builtins.c $(SRC_DIR)\errno_register.inc $(SRC_DIR)\builtins.inc
	$(MAKE) /$(MAKEFLAGS) /NOLOGO $(MINIYOG)
	$(MINIYOG) $(SRC_DIR)\yg2inc.yg $(SRC_DIR)\builtins.yg $(SRC_DIR)\builtins.inc
	$(CC) $(YOG_CFLAGS) /Fo$@ $(SRC_DIR)\builtins.c

$(SRC_DIR)\yog-class.obj: $(SRC_DIR)\class.c
	$(CC) $(YOG_CFLAGS) /Fo$@ $?

$(SRC_DIR)\yog-classmethod.obj: $(SRC_DIR)\classmethod.c
	$(CC) $(YOG_CFLAGS) /Fo$@ $?

$(SRC_DIR)\yog-code.obj: $(SRC_DIR)\code.c
	$(CC) $(YOG_CFLAGS) /Fo$@ $?

$(SRC_DIR)\yog-comparable.obj: $(SRC_DIR)\comparable.c
	$(CC) $(YOG_CFLAGS) /Fo$@ $?

$(SRC_DIR)\yog-compile.obj: $(SRC_DIR)\compile.c
	$(CC) $(YOG_CFLAGS) /Fo$@ $?

$(SRC_DIR)\yog-dict.obj: $(SRC_DIR)\dict.c $(SRC_DIR)\dict.inc
	$(CC) $(YOG_CFLAGS) /Fo$@ $(SRC_DIR)\dict.c

$(SRC_DIR)\dict.inc: $(SRC_DIR)\yg2inc.yg $(SRC_DIR)\dict.yg
	$(MAKE) /$(MAKEFLAGS) /NOLOGO $(MINIYOG)
	$(MINIYOG) $(SRC_DIR)\yg2inc.yg $(SRC_DIR)\dict.yg $@

$(SRC_DIR)\yog-encoding.obj: $(SRC_DIR)\encoding.c
	$(CC) $(YOG_CFLAGS) /Fo$@ $?

$(SRC_DIR)\yog-error.obj: $(SRC_DIR)\error.c $(SRC_DIR)\errno_raise.inc
	$(CC) $(YOG_CFLAGS) /Fo$@ $(SRC_DIR)\error.c

$(SRC_DIR)\errno_raise.inc: $(MINIYOG) $(TOOLS_DIR)\errno2raise.yg $(TOOLS_DIR)\errno
	$(MAKE) /$(MAKEFLAGS) /NOLOGO $(MINIYOG)
	$(MINIYOG) $(TOOLS_DIR)\errno2raise.yg $(TOOLS_DIR)\errno $@

$(SRC_DIR)\yog-eval.obj: $(SRC_DIR)\eval.c
	$(CC) $(YOG_CFLAGS) /Fo$@ $?

$(SRC_DIR)\eval.c: $(SRC_DIR)\eval.inc

$(SRC_DIR)\yog-exception.obj: $(SRC_DIR)\exception.c $(SRC_DIR)\errno_new.inc $(SRC_DIR)\errno_cons.inc
	$(CC) $(YOG_CFLAGS) /Fo$@ $(SRC_DIR)\exception.c

$(SRC_DIR)\errno_new.inc: $(MINIYOG) $(TOOLS_DIR)\errno2new.yg $(TOOLS_DIR)\errno
	$(MAKE) /$(MAKEFLAGS) /NOLOGO $(MINIYOG)
	$(MINIYOG) $(TOOLS_DIR)\errno2new.yg $(TOOLS_DIR)\errno $@

$(SRC_DIR)\errno_cons.inc: $(MINIYOG) $(TOOLS_DIR)\errno2cons.yg $(TOOLS_DIR)\errno
	$(MAKE) /$(MAKEFLAGS) /NOLOGO $(MINIYOG)
	$(MINIYOG) $(TOOLS_DIR)\errno2cons.yg $(TOOLS_DIR)\errno $@

$(SRC_DIR)\yog-file.obj: $(SRC_DIR)\file.c
	$(CC) $(YOG_CFLAGS) /Fo$@ $?

$(SRC_DIR)\yog-fixnum.obj: $(SRC_DIR)\fixnum.c
	$(CC) $(YOG_CFLAGS) /Fo$@ $?

$(SRC_DIR)\yog-float.obj: $(SRC_DIR)\float.c
	$(CC) $(YOG_CFLAGS) /Fo$@ $?

$(SRC_DIR)\yog-frame.obj: $(SRC_DIR)\frame.c
	$(CC) $(YOG_CFLAGS) /Fo$@ $?

$(SRC_DIR)\yog-callable.obj: $(SRC_DIR)\callable.c
	$(CC) $(YOG_CFLAGS) /Fo$@ $?

$(SRC_DIR)\yog-gc.obj: $(SRC_DIR)\gc.c
	$(CC) $(YOG_CFLAGS) /Fo$@ $?

$(SRC_DIR)\yog-inst.obj: $(SRC_DIR)\inst.c
	$(CC) $(YOG_CFLAGS) /Fo$@ $?

$(SRC_DIR)\yog-lexer.obj: $(SRC_DIR)\lexer.c
	$(CC) $(YOG_CFLAGS) /Fo$@ $?

$(SRC_DIR)\lexer.c: $(INCLUDE_DIR)\yog\token.h

$(INCLUDE_DIR)\yog\token.h: $(SRC_DIR)\parser.y $(LEMON_DIR)\lemon.exe
	$(LEMON_DIR)\lemon.exe header=$(INCLUDE_DIR)\yog\token.h $(SRC_DIR)\parser.y

$(SRC_DIR)\yog-main.obj: $(SRC_DIR)\main.c
	$(CC) $(YOG_CFLAGS) /Fo$@ $?

$(SRC_DIR)\yog-misc.obj: $(SRC_DIR)\misc.c
	$(CC) $(YOG_CFLAGS) /Fo$@ $?

$(SRC_DIR)\yog-module.obj: $(SRC_DIR)\module.c
	$(CC) $(YOG_CFLAGS) /Fo$@ $?

$(SRC_DIR)\yog-nil.obj: $(SRC_DIR)\nil.c
	$(CC) $(YOG_CFLAGS) /Fo$@ $?

$(SRC_DIR)\yog-object.obj: $(SRC_DIR)\object.c $(SRC_DIR)\object.inc
	$(CC) $(YOG_CFLAGS) /Fo$@ $(SRC_DIR)\object.c

$(SRC_DIR)\object.inc: $(SRC_DIR)\yg2inc.yg $(SRC_DIR)\object.yg
	$(MAKE) /$(MAKEFLAGS) /NOLOGO $(MINIYOG)
	$(MINIYOG) $(SRC_DIR)\yg2inc.yg $(SRC_DIR)\object.yg $@

$(SRC_DIR)\yog-package.obj: $(SRC_DIR)\package.c
	$(CC) $(YOG_CFLAGS) /Fo$@ $?

$(SRC_DIR)\yog-parser.obj: $(SRC_DIR)\parser.c
	$(CC) $(YOG_CFLAGS) /Fo$@ $?

$(SRC_DIR)\yog-property.obj: $(SRC_DIR)\property.c
	$(CC) $(YOG_CFLAGS) /Fo$@ $?

$(SRC_DIR)\yog-regexp.obj: $(SRC_DIR)\regexp.c
	$(CC) $(YOG_CFLAGS) /Fo$@ $?

$(SRC_DIR)\yog-repl.obj: $(SRC_DIR)\repl.c
	$(CC) $(YOG_CFLAGS) /Fo$@ $?

$(SRC_DIR)\yog-set.obj: $(SRC_DIR)\set.c
	$(CC) $(YOG_CFLAGS) /Fo$@ $?

$(SRC_DIR)\yog-stacktrace.obj: $(SRC_DIR)\stacktrace.c
	$(CC) $(YOG_CFLAGS) /Fo$@ $?

$(SRC_DIR)\yog-string.obj: $(SRC_DIR)\string.c $(SRC_DIR)\string.inc
	$(CC) $(YOG_CFLAGS) /Fo$@ $(SRC_DIR)\string.c

$(SRC_DIR)\string.inc: $(SRC_DIR)\yg2inc.yg $(SRC_DIR)\string.yg
	$(MAKE) /$(MAKEFLAGS) /NOLOGO $(MINIYOG)
	$(MINIYOG) $(SRC_DIR)\yg2inc.yg $(SRC_DIR)\string.yg $@

$(SRC_DIR)\yog-symbol.obj: $(SRC_DIR)\symbol.c
	$(CC) $(YOG_CFLAGS) /Fo$@ $?

$(SRC_DIR)\yog-table.obj: $(SRC_DIR)\table.c
	$(CC) $(YOG_CFLAGS) /Fo$@ $?

$(SRC_DIR)\yog-thread.obj: $(SRC_DIR)\thread.c
	$(CC) $(YOG_CFLAGS) /Fo$@ $?

$(SRC_DIR)\yog-value.obj: $(SRC_DIR)\value.c
	$(CC) $(YOG_CFLAGS) /Fo$@ $?

$(SRC_DIR)\yog-vm.obj: $(SRC_DIR)\vm.c $(INCLUDE_DIR)\yog\vm.h $(SRC_DIR)\errno_init.inc $(SRC_DIR)\errno_keep.inc
	$(CC) $(YOG_CFLAGS) /Fo$@ $(SRC_DIR)\vm.c

$(SRC_DIR)\errno_keep.inc: $(MINIYOG) $(TOOLS_DIR)\errno2keep.yg $(TOOLS_DIR)\errno
	$(MAKE) /$(MAKEFLAGS) /NOLOGO $(MINIYOG)
	$(MINIYOG) $(TOOLS_DIR)\errno2keep.yg $(TOOLS_DIR)\errno $@

$(INCLUDE_DIR)\yog\vm.h: $(INCLUDE_DIR)\yog\errno_vm.inc

$(SRC_DIR)\gc\yog-copying.obj: $(SRC_DIR)\gc\copying.c
	$(CC) $(YOG_CFLAGS) /Fo$@ $?

$(SRC_DIR)\yog-coroutine.obj: $(SRC_DIR)\coroutine.c
	$(CC) $(YOG_CFLAGS) /Fo$@ $?

$(SRC_DIR)\yog-getopt.obj: $(SRC_DIR)\getopt.c
	$(CC) $(YOG_CFLAGS) /Fo$@ $?

$(MINIYOG): $(MINIYOG_OBJS)
	$(LINK) $(LDFLAGS) /OUT:$@ $(LIBS) $(MINIYOG_OBJS)

$(SRC_DIR)\miniyog-arg.obj: $(SRC_DIR)\arg.c
	$(CC) $(MINIYOG_CFLAGS) /Fo$@ $?

$(SRC_DIR)\miniyog-array.obj: $(SRC_DIR)\array.c
	$(CC) $(MINIYOG_CFLAGS) /Fo$@ $?

$(SRC_DIR)\miniyog-bignum.obj: $(SRC_DIR)\bignum.c
	$(CC) $(MINIYOG_CFLAGS) /Fo$@ $?

$(SRC_DIR)\miniyog-binary.obj: $(SRC_DIR)\binary.c
	$(CC) $(MINIYOG_CFLAGS) /Fo$@ $?

$(SRC_DIR)\miniyog-bool.obj: $(SRC_DIR)\bool.c
	$(CC) $(MINIYOG_CFLAGS) /Fo$@ $?

$(SRC_DIR)\miniyog-builtins.obj: $(SRC_DIR)\builtins.c
	$(CC) $(MINIYOG_CFLAGS) /Fo$@ $?

$(SRC_DIR)\miniyog-class.obj: $(SRC_DIR)\class.c
	$(CC) $(MINIYOG_CFLAGS) /Fo$@ $?

$(SRC_DIR)\miniyog-classmethod.obj: $(SRC_DIR)\classmethod.c
	$(CC) $(MINIYOG_CFLAGS) /Fo$@ $?

$(SRC_DIR)\miniyog-code.obj: $(SRC_DIR)\code.c
	$(CC) $(MINIYOG_CFLAGS) /Fo$@ $?

$(SRC_DIR)\miniyog-comparable.obj: $(SRC_DIR)\comparable.c
	$(CC) $(MINIYOG_CFLAGS) /Fo$@ $?

$(SRC_DIR)\miniyog-compile.obj: $(SRC_DIR)\compile.c
	$(CC) $(MINIYOG_CFLAGS) /Fo$@ $?

$(SRC_DIR)\miniyog-dict.obj: $(SRC_DIR)\dict.c
	$(CC) $(MINIYOG_CFLAGS) /Fo$@ $?

$(SRC_DIR)\miniyog-encoding.obj: $(SRC_DIR)\encoding.c
	$(CC) $(MINIYOG_CFLAGS) /Fo$@ $?

$(SRC_DIR)\miniyog-error.obj: $(SRC_DIR)\error.c
	$(CC) $(MINIYOG_CFLAGS) /Fo$@ $?

$(SRC_DIR)\miniyog-eval.obj: $(SRC_DIR)\eval.c
	$(CC) $(MINIYOG_CFLAGS) /Fo$@ $(SRC_DIR)\eval.c

$(SRC_DIR)\eval.inc: $(SRC_DIR)\insts.def $(ROOT_DIR)\tools\inst.py
	$(PYTHON) $(ROOT_DIR)\tools\inst.py $(SRC_DIR)\insts.def $(ROOT_DIR)

$(SRC_DIR)\miniyog-exception.obj: $(SRC_DIR)\exception.c
	$(CC) $(MINIYOG_CFLAGS) /Fo$@ $?

$(SRC_DIR)\miniyog-file.obj: $(SRC_DIR)\file.c
	$(CC) $(MINIYOG_CFLAGS) /Fo$@ $?

$(SRC_DIR)\miniyog-fixnum.obj: $(SRC_DIR)\fixnum.c
	$(CC) $(MINIYOG_CFLAGS) /Fo$@ $?

$(SRC_DIR)\miniyog-float.obj: $(SRC_DIR)\float.c
	$(CC) $(MINIYOG_CFLAGS) /Fo$@ $?

$(SRC_DIR)\miniyog-frame.obj: $(SRC_DIR)\frame.c
	$(CC) $(MINIYOG_CFLAGS) /Fo$@ $?

$(SRC_DIR)\miniyog-callable.obj: $(SRC_DIR)\callable.c
	$(CC) $(MINIYOG_CFLAGS) /Fo$@ $?

$(SRC_DIR)\miniyog-gc.obj: $(SRC_DIR)\gc.c
	$(CC) $(MINIYOG_CFLAGS) /Fo$@ $?

$(SRC_DIR)\miniyog-inst.obj: $(SRC_DIR)\inst.c
	$(CC) $(MINIYOG_CFLAGS) /Fo$@ $?

$(SRC_DIR)\inst.c: $(SRC_DIR)\inst.c.tmpl $(ROOT_DIR)\tools\inst.py $(SRC_DIR)\insts.def
	$(PYTHON) $(ROOT_DIR)\tools\inst.py $(SRC_DIR)\insts.def $(ROOT_DIR)

$(SRC_DIR)\miniyog-lexer.obj: $(SRC_DIR)\lexer.c
	$(CC) $(MINIYOG_CFLAGS) /Fo$@ $?

$(SRC_DIR)\miniyog-main.obj: $(SRC_DIR)\main.c
	$(CC) $(MINIYOG_CFLAGS) /Fo$@ $?

$(SRC_DIR)\miniyog-misc.obj: $(SRC_DIR)\misc.c
	$(CC) $(MINIYOG_CFLAGS) /Fo$@ $?

$(SRC_DIR)\miniyog-module.obj: $(SRC_DIR)\module.c
	$(CC) $(MINIYOG_CFLAGS) /Fo$@ $?

$(SRC_DIR)\miniyog-nil.obj: $(SRC_DIR)\nil.c
	$(CC) $(MINIYOG_CFLAGS) /Fo$@ $?

$(SRC_DIR)\miniyog-object.obj: $(SRC_DIR)\object.c
	$(CC) $(MINIYOG_CFLAGS) /Fo$@ $?

$(SRC_DIR)\miniyog-package.obj: $(SRC_DIR)\package.c
	$(CC) $(MINIYOG_CFLAGS) /Fo$@ $?

$(SRC_DIR)\miniyog-parser.obj: $(SRC_DIR)\parser.c
	$(CC) $(MINIYOG_CFLAGS) /Fo$@ $?

$(SRC_DIR)\parser.c: $(SRC_DIR)\parser.lt $(SRC_DIR)\parser.y $(LEMON_DIR)\lemon.exe
	$(LEMON_DIR)\lemon.exe header=$(INCLUDE_DIR)\yog\token.h $(SRC_DIR)\parser.y

$(LEMON_DIR)\lemon.exe: $(LEMON_DIR)\lemon.c
	$(CC) /Fe$@ /Wall /nologo /O2 /Z7 /MT $(LEMON_DIR)\lemon.c

$(SRC_DIR)\miniyog-property.obj: $(SRC_DIR)\property.c
	$(CC) $(MINIYOG_CFLAGS) /Fo$@ $?

$(SRC_DIR)\miniyog-regexp.obj: $(SRC_DIR)\regexp.c
	$(CC) $(MINIYOG_CFLAGS) /Fo$@ $?

$(SRC_DIR)\miniyog-repl.obj: $(SRC_DIR)\repl.c
	$(CC) $(MINIYOG_CFLAGS) /Fo$@ $?

$(SRC_DIR)\miniyog-set.obj: $(SRC_DIR)\set.c
	$(CC) $(MINIYOG_CFLAGS) /Fo$@ $?

$(SRC_DIR)\miniyog-stacktrace.obj: $(SRC_DIR)\stacktrace.c
	$(CC) $(MINIYOG_CFLAGS) /Fo$@ $?

$(SRC_DIR)\miniyog-string.obj: $(SRC_DIR)\string.c
	$(CC) $(MINIYOG_CFLAGS) /Fo$@ $?

$(SRC_DIR)\miniyog-symbol.obj: $(SRC_DIR)\symbol.c
	$(CC) $(MINIYOG_CFLAGS) /Fo$@ $?

$(SRC_DIR)\miniyog-table.obj: $(SRC_DIR)\table.c
	$(CC) $(MINIYOG_CFLAGS) /Fo$@ $?

$(SRC_DIR)\miniyog-thread.obj: $(SRC_DIR)\thread.c
	$(CC) $(MINIYOG_CFLAGS) /Fo$@ $?

$(SRC_DIR)\miniyog-value.obj: $(SRC_DIR)\value.c
	$(CC) $(MINIYOG_CFLAGS) /Fo$@ $?

$(SRC_DIR)\miniyog-vm.obj: $(SRC_DIR)\vm.c
	$(CC) $(MINIYOG_CFLAGS) /Fo$@ $?

$(SRC_DIR)\gc\miniyog-copying.obj: $(SRC_DIR)\gc\copying.c
	$(CC) $(MINIYOG_CFLAGS) /Fo$@ $?

$(SRC_DIR)\miniyog-coroutine.obj: $(SRC_DIR)\coroutine.c
	$(CC) $(MINIYOG_CFLAGS) /Fo$@ $?

$(SRC_DIR)\miniyog-getopt.obj: $(SRC_DIR)\getopt.c
	$(CC) $(MINIYOG_CFLAGS) /Fo$@ $?

icon.RES: icon.rc yog.ico
	rc icon.rc

$(SRC_DIR)\yog-get_args.obj: $(SRC_DIR)\get_args.c
	$(CC) $(YOG_CFLAGS) /Fo$@ $?

$(SRC_DIR)\miniyog-get_args.obj: $(SRC_DIR)\get_args.c
	$(CC) $(MINIYOG_CFLAGS) /Fo$@ $?

$(SRC_DIR)\yog-sprintf.obj: $(SRC_DIR)\sprintf.c
	$(CC) $(YOG_CFLAGS) /Fo$@ $?

$(SRC_DIR)\miniyog-sprintf.obj: $(SRC_DIR)\sprintf.c
	$(CC) $(MINIYOG_CFLAGS) /Fo$@ $?

$(INCLUDE_DIR)\yog\errno_vm.inc: $(MINIYOG) $(TOOLS_DIR)\errno2vm.yg $(TOOLS_DIR)\errno
	$(MAKE) /$(MAKEFLAGS) /NOLOGO $(MINIYOG)
	$(MINIYOG) $(TOOLS_DIR)\errno2vm.yg $(TOOLS_DIR)\errno $@

$(SRC_DIR)\errno_init.inc: $(MINIYOG) $(TOOLS_DIR)\errno2init.yg $(TOOLS_DIR)\errno
	$(MAKE) /$(MAKEFLAGS) /NOLOGO $(MINIYOG)
	$(MINIYOG) $(TOOLS_DIR)\errno2init.yg $(TOOLS_DIR)\errno $@

$(SRC_DIR)\errno_register.inc: $(MINIYOG) $(TOOLS_DIR)\errno2register.yg $(TOOLS_DIR)\errno
	$(MAKE) /$(MAKEFLAGS) /NOLOGO $(MINIYOG)
	$(MINIYOG) $(TOOLS_DIR)\errno2register.yg $(TOOLS_DIR)\errno $@

clean:
	del $(SRC_DIR)\*.obj *.exe

# vim: tabstop=8 shiftwidth=8 noexpandtab
