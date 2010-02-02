# -*- coding: utf-8 -*-

from testcase import TestCase

class TestPeg(TestCase):

    def test_string_match0(self):
        self._test("""
import peg
print(peg.pattern(\"foo\").match(\"foo\") != nil)
""", "true")

    def test_string_match10(self):
        self._test("""
import peg
print(peg.pattern(\"foo\").match(\"bar\"))
""", "nil")

# vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
