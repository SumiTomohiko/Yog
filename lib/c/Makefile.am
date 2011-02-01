
YOG = $(top_srcdir)/src/yog-generational
LALR1GRAM = $(top_srcdir)/bin/lalr1gram.yog
SRC = grammar.yogram

bin_PROGRAMS = grammar.yg
grammar_yg_SOURCES = $(SRC)

grammar.yg: $(SRC) $(LALR1GRAM)
	$(YOG) --young-heap-size=32M $(LALR1GRAM) $@ $(SRC)

# vim: tabstop=8 shiftwidth=8 noexpandtab
