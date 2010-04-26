
SUBDIRS = @BDWGC_DIR@ onig gmp src ext
ARCHIVE = Yog-@PACKAGE_VERSION@
SYNTAX_DIR = $(prefix)/share/vim/vim71/syntax

.PHONY: doc proto test test-installed test-copying test-mark-sweep \
	test-mark-sweep-compact test-bdw test-generational install-vim install \
	dist distcheck installer

doc:
	cd doc && make

proto:
	$(PYTHON) tools/update_prototype.py

test:
	@for t in @TEST_TARGETS@; do \
		$(MAKE) $${t}; \
	done

test-installed:
	export YOG=$(prefix)/bin/yog-generational$(EXEEXT); \
	./run_tests

test-copying:
	export YOG=`pwd`/src/yog-copying$(EXEEXT); \
	./run_tests

test-mark-sweep:
	export YOG=`pwd`/src/yog-mark-sweep$(EXEEXT); \
	./run_tests

test-mark-sweep-compact:
	export YOG=`pwd`/src/yog-mark-sweep-compact$(EXEEXT); \
	./run_tests

test-bdw:
	export YOG=`pwd`/src/yog-bdw$(EXEEXT); \
	./run_tests

test-generational:
	export YOG=`pwd`/src/yog-generational$(EXEEXT); \
	./run_tests

install-vim:
	mkdir -p $(SYNTAX_DIR)
	cp yog.vim $(SYNTAX_DIR)
	@grep "NOTICE" yog.vim | sed -e 's/" NOTICE: //'

install:
	for d in src lib ext; do \
		(cd $$d && $(MAKE) $@) \
	done

pkg:
	tools/dist --python=$(PYTHON) $(ARCHIVE)

pkgcheck:
	tools/distcheck $(ARCHIVE)

installer: installer.nsi
	makensis -V4 -P5 installer.nsi

configure.ac: yog.m4
	autoconf

# vim: tabstop=8 shiftwidth=8 noexpandtab filetype=automake
