
YOG = $(top_srcdir)/src/yog-generational

bin_PROGRAMS = grammar.yg
grammar_yg_SOURCES = grammar.yogram

grammar.yg: grammar.yogram
	$(top_srcdir)/src/yog-generational --young-heap-size=32M $(top_srcdir)/bin/lalr1gram.yog $@ $<

# vim: tabstop=8 shiftwidth=8 noexpandtab
