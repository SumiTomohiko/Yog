
SUBDIRS = c

DIR = $(prefix)/lib/yog/$(VERSION)

install:
	mkdir -p $(DIR)
	for f in `find . -name "*.yg"`; \
	do \
		mkdir -p `dirname $(DIR)/$${f}`; \
		$(install_sh_DATA) $${f} $(DIR)/$${f}; \
	done

# vim: tabstop=8 shiftwidth=8 noexpandtab
