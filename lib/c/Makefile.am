
YOG = $(top_srcdir)/build/src/yog
LALR1GRAM = $(top_srcdir)/bin/lalr1gram.yog
SRC = grammar.yogram

bin_PROGRAMS = grammar.yog
grammar_yog_SOURCES = $(SRC)

grammar.yog: $(SRC) $(LALR1GRAM)
	$(YOG) $(LALR1GRAM) $@ $(SRC)

# vim: tabstop=8 shiftwidth=8 noexpandtab
