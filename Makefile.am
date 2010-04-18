
SUBDIRS = @DIRS@
ARCHIVE = Yog-@PACKAGE_VERSION@
SYNTAX_DIR = $(prefix)/share/vim/vim71/syntax

.PHONY: doc proto test test-installed test-copying test-mark-sweep \
	test-mark-sweep-compact test-bdw test-generational install-vim install \
	dist distcheck installer

doc:
	cd doc && make

proto:
	$(PYTHON) tools/update_prototype.py

test test-installed test-copying test-mark-sweep test-mark-sweep-compact test-bdw test-generational:
	cd tests && $(MAKE) $@

install-vim:
	mkdir -p $(SYNTAX_DIR)
	cp yog.vim $(SYNTAX_DIR)
	@grep "NOTICE" yog.vim | sed -e 's/" NOTICE: //'

install:
	$(install_sh) -m 755 src/yog-copying$(EXEEXT) $(prefix)/bin/yog$(EXEEXT)
	for d in lib ext; do \
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
