
INSTALL_SUBDIRS = bin src ext lib
SUBDIRS = corgi gmp libffi $(INSTALL_SUBDIRS)
ARCHIVE = $(PACKAGE_NAME)-$(PACKAGE_VERSION)

.PHONY: doc proto test test-installed test-copying test-mark-sweep \
	test-mark-sweep-compact vim install dist distcheck

doc:
	cd doc && make

proto:
	$(PYTHON) tools/update_prototype.py

test-installed:
	export YOG=$(prefix)/bin/yog$(EXEEXT); \
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

test:
	export YOG=`pwd`/src/yog$(EXEEXT); \
	./run_tests

VIM_DIR = vim
vim:
	cd $(VIM_DIR) && $(MAKE)

install-vim: vim
	cd $(VIM_DIR) && $(MAKE) install

install:
	for d in $(INSTALL_SUBDIRS); do \
		(cd $$d && $(MAKE) $@) \
	done

pkg:
	tools/dist -p $(PYTHON) $(ARCHIVE)

pkgcheck:
	tools/distcheck $(ARCHIVE)

configure.ac: yog.m4
	autoconf

deb:
	dpkg-buildpackage

# vim: tabstop=8 shiftwidth=8 noexpandtab filetype=automake
