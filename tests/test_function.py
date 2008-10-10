# -*- coding: utf-8 -*-

from tests import TestCase

class TestFunction(TestCase):

    def test_func(self):
        self._test("""
def foo()
    puts 42
end

foo()
""", "42\n")

    def test_param(self):
        self._test("""
def foo(a)
    puts a
end

foo(42)
""", "42\n")

# vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
