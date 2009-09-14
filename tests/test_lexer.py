# -*- coding: utf-8 -*-

from re import match
from testcase import TestCase

class TestLexer(TestCase):

    def test_regexp0(self):
        def test_stderr(stderr):
            m = match(r"""SyntaxError: EOL while scanning regexp literal
""", stderr)
            assert m is not None

        self._test("""
/\\""", stderr=test_stderr)

    def test_string0(self):
        def test_stderr(stderr):
            m = match(r"""SyntaxError: EOL while scanning string literal
""", stderr)
            assert m is not None

        self._test("""
\"\\""", stderr=test_stderr)

    def test_string10(self):
        def test_stderr(stderr):
            m = match(r"""SyntaxError: invalid multibyte char
""", stderr)
            assert m is not None

        self._test(u"""
\"\xe7""", stderr=test_stderr)

    def test_comment0(self):
        self._test("""
($ print(42 $)
print(26)
""", "26")

    def test_comment10(self):
        self._test("""
print ($ print(42 $) (26)
""", "26")

    def test_comment20(self):
        self._test("""
($
 $ print(42)
 $)
print(26)
""", "26")

# vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
