
SYNTAX_DIR = $(prefix)/share/vim/vim71/syntax
FILES = yog.vim yogram.vim

all:
	$(mkdir_p) $(SYNTAX_DIR)
	for f in $(FILES); \
	do \
		$(INSTALL_DATA) $${f} $(SYNTAX_DIR); \
	done

# vim: tabstop=8 shiftwidth=8 noexpandtab filetype=automake
