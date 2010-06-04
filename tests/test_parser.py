# -*- coding: utf-8 -*-

from testcase import TestCase

class TestParser(TestCase):

    def test_duplicate_argument_name(self):
        def test_stderr(stderr):
            self._test_regexp("SyntaxError: file \"[^\"]+\", line 2: Duplicated argument name in function definition\n", stderr)

        self._test("""
def foo(bar, bar)
end
""", stderr=test_stderr)

    def test_SyntaxError0(self):
        def test_stderr(stderr):
            self._test_regexp(r"""SyntaxError: file "[^"]+", line 2: invalid syntax
""", stderr)

        self._test("""
def def
""", stderr=test_stderr)

    def test_regexp_after_if0(self):
        self._test("""
if //
  print(42)
end
""", "42")

    def test_regexp_after_if10(self):
        self._test("""
if /foo/
  print(42)
end
""", "42")

# vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
