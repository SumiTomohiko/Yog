# -*- coding: utf-8 -*-

from testcase import TestCase

class TestFunction(TestCase):

    def test_func(self):
        self._test("""
def foo()
    puts(42)
end

foo()
""", "42\n")

    def test_param(self):
        self._test("""
def foo(a)
    puts(a)
end

foo(42)
""", "42\n")

    def test_no_return(self):
        self._test("""
def foo()
end

puts(foo())
""", """nil
""")

    def test_uncallable0(self):
        def test_stderr(stderr):
            self._test_regexp(r"""Traceback \(most recent call last\):
  File "[^"]+", line 2, in <package>
TypeError: Fixnum is not callable
""", stderr)
        self._test("""
42()
""", stderr=test_stderr)

# vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
