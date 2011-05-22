
SUBDIRS = c sdl

bin_SCRIPTS = sdl.yg

include $(top_srcdir)/lib/Makefile.common

libdir = $(DESTDIR)$(prefix)/lib/yog/$(PACKAGE_VERSION)
install:
	for f in `find . -name "*.yg"`; \
	do \
		$(mkdir_p) `dirname $(libdir)/$${f}`; \
		$(install_sh_DATA) $${f} $(libdir)/$${f}; \
	done

# vim: tabstop=8 shiftwidth=8 noexpandtab
