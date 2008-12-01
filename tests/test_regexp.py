# -*- coding: utf-8 -*-

from tests import TestCase

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
s = "foo"
if s =~ /foo/
  puts 42
end""", """42
""")

# vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
