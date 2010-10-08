
SUBDIRS = src
ACLOCAL_AMFLAGS = -I m4
ARCHIVE_NAME = $(PACKAGE_NAME)-$(PACKAGE_VERSION)

doc:
	if [ ! -h doc/index.rst ]; then \
		ln -s `pwd`/README `pwd`/doc/index.rst; \
	fi
	cd doc && make html

pkg:
	tools/dist -p $(PYTHON) $(ARCHIVE_NAME)

pkgcheck:
	tools/distcheck $(ARCHIVE_NAME)

.PHONY: doc pkg pkgcheck

# vim: tabstop=8 shiftwidth=8 noexpandtab
