
SUBDIRS = c sdl

bin_SCRIPTS = sdl.yg

include $(top_srcdir)/lib/Makefile.common

install:
	for f in `find . -name "*.yg"`; \
	do \
		$(install_sh_DATA) $${f} $(libdir)/$${f}; \
	done

# vim: tabstop=8 shiftwidth=8 noexpandtab
