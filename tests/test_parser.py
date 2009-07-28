# -*- coding: utf-8 -*-

from re import match
from testcase import TestCase

class TestParser(TestCase):

    def test_duplicate_argument_name(self):
        def test_stderr(stderr):
            m = match("SyntaxError: file \"[^\"]+\", line 2: duplicated argument name in function definition\n", stderr)
            assert m is not None

        self._test("""
def foo(bar, bar)
end
""", stderr=test_stderr)

    def test_SyntaxError0(self):
        def test_stderr(stderr):
            m = match(r"""SyntaxError: file "[^"]+", line 2: invalid syntax
""", stderr)
            assert m is not None

        self._test("""
def def
""", stderr=test_stderr)

# vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
