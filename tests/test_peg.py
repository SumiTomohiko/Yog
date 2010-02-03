# -*- coding: utf-8 -*-

from testcase import TestCase

class TestPeg(TestCase):

    def test_string0(self):
        self._test("""
import peg
print(peg.pattern(\"foo\").match(\"foo\") != nil)
""", "true")

    def test_string10(self):
        self._test("""
import peg
print(peg.pattern(\"foo\").match(\"bar\"))
""", "nil")

    def test_string_serial0(self):
        self._test("""
import peg
print((peg.pattern(\"foo\") * peg.pattern(\"bar\")).match(\"foobar\") != nil)
""", "true")

    def test_string_serial10(self):
        self._test("""
import peg
print((peg.pattern(\"foo\") * peg.pattern(\"bar\")).match(\"quux\"))
""", "nil")

    def test_string_choice0(self):
        self._test("""
import peg
print((peg.pattern(\"foo\") / peg.pattern(\"bar\")).match(\"foo\") != nil)
""", "true")

    def test_string_choice10(self):
        self._test("""
import peg
print((peg.pattern(\"foo\") / peg.pattern(\"bar\")).match(\"quux\"))
""", "nil")

    def test_string_at_least5(self):
        self._test("""
import peg
print((peg.pattern(\"foo\") ^ 0).match(\"\") != nil)
""", "true")

    def test_string_at_least0(self):
        self._test("""
import peg
print((peg.pattern(\"foo\") ^ 0).match(\"foo\") != nil)
""", "true")

    def test_string_at_least10(self):
        self._test("""
import peg
print((peg.pattern(\"foo\") ^ 0).match(\"foofoo\") != nil)
""", "true")

    def test_string_at_least20(self):
        self._test("""
import peg
print((peg.pattern(\"foo\") ^ 0).match(\"baz\"))
""", "nil")

    def test_string_at_least25(self):
        self._test("""
import peg
print((peg.pattern(\"foo\") ^ 1).match(\"\"))
""", "nil")

    def test_string_at_least30(self):
        self._test("""
import peg
print((peg.pattern(\"foo\") ^ 1).match(\"foo\") != nil)
""", "true")

    def test_string_at_least40(self):
        self._test("""
import peg
print((peg.pattern(\"foo\") ^ 1).match(\"foofoo\") != nil)
""", "true")

    def test_string_at_least50(self):
        self._test("""
import peg
print((peg.pattern(\"foo\") ^ 1).match(\"baz\"))
""", "nil")

    def test_string_at_most0(self):
        self._test("""
import peg
print((peg.pattern(\"foo\") ^ (-1)).match(\"\") != nil)
""", "true")

    def test_string_at_most10(self):
        self._test("""
import peg
print((peg.pattern(\"foo\") ^ (-1)).match(\"foo\") != nil)
""", "true")

    def test_string_at_most20(self):
        self._test("""
import peg
print((peg.pattern(\"foo\") ^ (-1)).match(\"foofoo\") != nil)
""", "true")

    def test_string_at_most30(self):
        self._test("""
import peg
print((peg.pattern(\"foo\") ^ (-1)).match(\"bar\"))
""", "nil")

# vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
