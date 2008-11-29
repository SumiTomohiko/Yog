# -*- coding: utf-8 -*-

from tests import TestCase

class TestCommand(TestCase):

    def test_command1(self):
        self._test("""
def foo()
  puts 42
end

foo""", """42
""")

    def test_command2(self):
        self._test("""
def foo(n)
  puts n
end

foo 42""", """42
""")

    def test_args(self):
        self._test("puts 42, 43", """42
43
""")

# vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
