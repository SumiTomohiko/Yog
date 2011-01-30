
bin_PROGRAMS = corgi
corgi_SOURCES = main.c
corgi_LDADD = .libs/libcorgi.a
lib_LTLIBRARIES = libcorgi.la
libcorgi_la_SOURCES = corgi.c unicode.c
DEFAULT_INCLUDES = -I$(top_srcdir)/include
AM_CFLAGS = -Wall -Werror -g

TOOLS_DIR = $(top_srcdir)/tools
UNICODE_SRC = UnicodeData.txt Unihan.txt DerivedCoreProperties.txt \
	      LineBreak.txt $(TOOLS_DIR)/makeunicodedata.py

entries.inc indexes.inc linebreaks.inc whitespaces.inc: $(UNICODE_SRC)
	$(PYTHON) $(TOOLS_DIR)/makeunicodedata.py .

$(top_srcdir)/include/corgi/constants.h: $(TOOLS_DIR)/constants.py
	$(PYTHON) $(TOOLS_DIR)/constants.py $@

# vim: tabstop=8 shiftwidth=8 noexpandtab
