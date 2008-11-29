# -*- coding: utf-8 -*-

from re import match

from tests import TestCase

class TestString(TestCase):

    def test_literal1(self):
        self._test("""
puts \"foo\"""", """foo
""")

    def test_literal2(self):
        self._test("""
puts \"\"""", """
""")

    def test_literal3(self):
        self._test("""
puts \"'\"""", """'
""")

    def test_literal4(self):
        self._test("""
puts 'foo'""", """foo
""")

    def test_literal5(self):
        self._test("""
puts ''""", """
""")

    def test_literal6(self):
        self._test("""
puts '\"'""", """\"
""")

    def test_literal7(self):
        self._test("""
puts \"\\n\"""", """

""")

    def test_escape1(self):
        self._test("""
puts \"\\\"\"""", """\"
""")

    def test_escape2(self):
        self._test("""
puts \"\\\\\"""", """\\
""")

    def test_escape3(self):
        self._test("""
puts '\\''""", """'
""")

    def test_escape4(self):
        self._test("""
puts '\\\\'""", """\\
""")

    def test_escape5(self):
        def test_stderr(stderr):
            m = match(r"""Traceback \(most recent call last\):
  File "[^"]+", line 2, in <module>
ParseError: unknown escape sequence (\\w)
""", stderr)
            assert m is not None

        self._test("""
puts \"\\w\"""", stderr=test_stderr)

    def test_add(self):
        self._test("""
puts \"foo\" + \"bar\"""", """foobar
""")

    def test_lshift1(self):
        self._test("""
s = \"foo\"
puts s << \"bar\"""", """foobar
""")

    def test_lshift2(self):
        self._test("""
s = \"foo\"
s << \"bar\"
puts s""", """foobar
""")

    def test_each_char1(self):
        self._test("""
\"foo\".each_char() do [c]
  puts c
end""", """f
o
o
""")

    def test_each_char2(self):
        self._test("""
\"日本語\".each_char() do [c]
  puts c
end""", """日
本
語
""")

    def test_each_byte1(self):
        self._test("""
\"foo\".each_byte() do [b]
  puts b
end""", """102
102
111
""")

    def test_each_byte2(self):
        self._test("""
\"日本語\".each_byte() do [b]
  puts b
end""", """230
151
165
230
156
172
232
170
158
""")

    def test_each_line1(self):
        self._test("""
\"foo\".each_line() do [l]
  puts l
end""", """foo
""")

    def test_each_line2(self):
        self._test("""
\"foo\nbar\".each_line() do [l]
  puts l
end""", """foo
bar
""")

    def test_index1(self):
        self._test("""
s = \"foo\"
s[0] = \"b\"
puts s
""", """boo
""")

    def test_index2(self):
        self._test("""
s = \"foo\"
s[0] = \"燦\"
puts s""", """燦oo
""")

    def test_index3(self):
        self._test("""
s = \"foo\"
puts s[0]""", """f
""")

# vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
