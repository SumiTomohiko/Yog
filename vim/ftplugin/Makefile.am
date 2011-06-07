
include $(top_srcdir)/vim/Makefile.common

datadir = $(VIM_DIR)/ftplugin
data_DATA = yg.vim yog.vim yogram.vim

yg.vim: yog.vim
	ln -s yog.vim $@

# vim: tabstop=8 shiftwidth=8 noexpandtab filetype=automake
