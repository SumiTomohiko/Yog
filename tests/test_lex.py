# -*- coding: utf-8 -*-

from testcase import TestCase

class TestLex(TestCase):

    def test_lex0(self):
        self._test("""from lex import Lexer
lexer = Lexer.new()
lexer.add_rule(\"foo\") do
  next 42
end
lexer.tokenize(\"foo\")
print(lexer.next())""", "42")

    def test_lex10(self):
        self._test("""from lex import Lexer
lexer = Lexer.new()
lexer.tokenize(\"\")
print(lexer.next().inspect())""", "'eof")

    def test_lex20(self):
        self._test("""from lex import Lexer
lexer = Lexer.new()
lexer.add_rule(\"foo\") do
  next 42
end
lexer.tokenize(\"foo\")
lexer.next()
print(lexer.next().inspect())""", "'eof")

    def test_lex30(self):
        self._test("""from lex import Lexer
lexer = Lexer.new()
lexer.add_rule(\"foo\") do |matched|
  next matched
end
lexer.tokenize(\"foo\")
print(lexer.next())""", "foo")

    def test_lex40(self):
        self._test("""from lex import Lexer
lexer = Lexer.new()
lexer.add_rule(\"foo\") do
end
lexer.add_rule(\"bar\") do
  next 42
end
lexer.tokenize(\"foobar\")
lexer.next()
print(lexer.next())""", "42")

# vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
