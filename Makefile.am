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
	src/vm.c \
	onig/regcomp.c \
	onig/regenc.c \
	onig/regerror.c \
	onig/regexec.c \
	onig/regext.c \
	onig/reggnu.c \
	onig/regparse.c \
	onig/regposerr.c \
	onig/regposix.c \
	onig/regsyntax.c \
	onig/regtrav.c \
	onig/regversion.c \
	onig/st.c \
	onig/enc/ascii.c \
	onig/enc/big5.c \
	onig/enc/cp1251.c \
	onig/enc/euc_jp.c \
	onig/enc/euc_kr.c \
	onig/enc/euc_tw.c \
	onig/enc/gb18030.c \
	onig/enc/iso8859_1.c \
	onig/enc/iso8859_10.c \
	onig/enc/iso8859_11.c \
	onig/enc/iso8859_13.c \
	onig/enc/iso8859_14.c \
	onig/enc/iso8859_15.c \
	onig/enc/iso8859_16.c \
	onig/enc/iso8859_2.c \
	onig/enc/iso8859_3.c \
	onig/enc/iso8859_4.c \
	onig/enc/iso8859_5.c \
	onig/enc/iso8859_6.c \
	onig/enc/iso8859_7.c \
	onig/enc/iso8859_8.c \
	onig/enc/iso8859_9.c \
	onig/enc/koi8_r.c \
	onig/enc/sjis.c \
	onig/enc/unicode.c \
	onig/enc/utf16_be.c \
	onig/enc/utf16_le.c \
	onig/enc/utf32_be.c \
	onig/enc/utf32_le.c \
	onig/enc/utf8.c
yog_CFLAGS = -Iinclude -Ionig -Wall
AM_YFLAGS = -d --verbose
#AM_LFLAGS = --noline

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
