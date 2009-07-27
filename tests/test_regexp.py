# -*- coding: utf-8 -*-

from re import match
from testcase import TestCase

class TestRegexp(TestCase):

    def test_literal10(self):
        self._test("""
s = \"\"
s =~ //""")

    def test_literal20(self):
        self._test("""
s = \"\"
s =~ //i""")

    def test_literal30(self):
        self._test("""
s = \"\"
s =~ /foo/i""")

    def test_match_expr10(self):
        self._test("""
s = \"foo\"
if s =~ /foo/
  puts(42)
end""", """42
""")

    def test_match_expr15(self):
        self._test("""
s = \"foobarbaz\"
if s =~ /bar/
  puts(42)
end""", """42
""")

    def test_match_expr20(self):
        self._test("""
s = \"foo\"
if s =~ /bar/
else
  puts(42)
end""", """42
""")

    def test_match_ignore_case10(self):
        self._test("""
s = \"foo\"
if s =~ /FOO/i
  puts(42)
end""", """42
""")

    def test_match_group05(self):
        self._test("""
m = \"foo\" =~ /foo/
puts(m.group())
""", """foo
""")

    def test_match_group10(self):
        self._test("""
m = \"foo\" =~ /foo/
puts(m.group(0))
""", """foo
""")

    def test_match_group15(self):
        self._test("""
m = \"foobarbaz\" =~ /bar/
puts(m.group())
""", """bar
""")

    def test_match_group20(self):
        self._test("""
m = \"foobarbaz\" =~ /bar/
puts(m.group(0))
""", """bar
""")

    def test_match_group30(self):
        self._test("""
m = \"foobarbazquux\" =~ /foo(bar)baz/
puts(m.group(1))
""", """bar
""")

    def test_match_group40(self):
        self._test("""
m = \"foobarbazquux\" =~ /foo(?<name>bar)baz/
puts(m.group(\"name\"))
""", """bar
""")

    def test_match_start05(self):
        self._test("""
m = \"foo\" =~ /foo/
puts(m.start())
""", """0
""")

    def test_match_start10(self):
        self._test("""
m = \"foo\" =~ /foo/
puts(m.start(0))
""", """0
""")

    def test_match_start15(self):
        self._test("""
m = \"foobarbaz\" =~ /bar/
puts(m.start())
""", """3
""")

    def test_match_start20(self):
        self._test("""
m = \"foobarbaz\" =~ /bar/
puts(m.start(0))
""", """3
""")

    def test_match_start30(self):
        self._test("""
m = \"foobarbazquux\" =~ /foo(bar)baz/
puts(m.start(1))
""", """3
""")

    def test_match_start40(self):
        self._test("""
m = \"foobarbazquux\" =~ /foo(?<name>bar)baz/
puts(m.start(\"name\"))
""", """3
""")

    def test_match_end05(self):
        self._test("""
m = \"foo\" =~ /foo/
puts(m.end())
""", """3
""")

    def test_match_end10(self):
        self._test("""
m = \"foo\" =~ /foo/
puts(m.end(0))
""", """3
""")

    def test_match_end15(self):
        self._test("""
m = \"foobarbaz\" =~ /bar/
puts(m.end())
""", """6
""")

    def test_match_end20(self):
        self._test("""
m = \"foobarbaz\" =~ /bar/
puts(m.end(0))
""", """6
""")

    def test_match_end30(self):
        self._test("""
m = \"foobarbazquux\" =~ /foo(bar)baz/
puts(m.end(1))
""", """6
""")

    def test_match_end40(self):
        self._test("""
m = \"foobarbazquux\" =~ /foo(?<name>bar)baz/
puts(m.end(\"name\"))
""", """6
""")

    def test_match_end50(self):
        def test_stderr(stderr):
            m = match(r"""Traceback \(most recent call last\):
  File "[^"]+", line 3, in <module>
  File builtin, in Match#end
IndexError: no such group
""", stderr)
            assert m is not None

        self._test("""
m = \"foo\" =~ /foo/
puts(m.end(42))
""", stderr=test_stderr)

# vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
