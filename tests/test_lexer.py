# -*- coding: utf-8 -*-

from testcase import TestCase

class TestLexer(TestCase):

    def test_regexp0(self):
        def test_stderr(stderr):
            self._test_regexp(r"""SyntaxError: EOL while scanning regexp literal
""", stderr)

        self._test("""
/\\""", stderr=test_stderr)

    def test_string0(self):
        def test_stderr(stderr):
            self._test_regexp(r"""SyntaxError: EOL while scanning string literal
""", stderr)

        self._test("""
\"\\""", stderr=test_stderr)

    def test_string10(self):
        """
        Test invalid multibyte charactor.
        """
        def test_stderr(stderr):
            self._test_regexp(r"""SyntaxError: invalid multibyte char
""", stderr)

        # Source must be the string (not Unicode).
        self._test("""
\"\xe7""", stderr=test_stderr, encoding=None)

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
