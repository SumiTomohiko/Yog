# -*- coding: utf-8 -*-

from testcase import TestCase

class TestLexer(TestCase):

    def test_unsupported_encoding(self):
        def test_stderr(stderr):
            self._test_regexp("SyntaxError: Unsupported encoding: foo", stderr)
        self._test("# -*- coding: foo -*-", stderr=test_stderr)

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

#    def test_string10(self):
#        """
#        Test invalid multibyte charactor.
#        """
#        def test_stderr(stderr):
#            self._test_regexp(r"""SyntaxError: invalid multibyte char
#""", stderr)
#
#        # Source must be the string (not Unicode).
#        self._test("""
#\"\xe7""", stderr=test_stderr, encoding=None)

    def test_parenthesis_balance0(self):
        self._test("""
[
42,
26
]
""", "")

    def test_parenthesis_balance10(self):
        self._test("""
{
42,
26
}
""", "")

    def test_parenthesis_balance20(self):
        self._test("""
(
42
)
""", "")

# vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
