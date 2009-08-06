# -*- coding: utf-8 -*-

from testcase import TestCase

class TestLogicalOp(TestCase):

    def test_and0(self):
        self._test("""
puts(42 && 26)
""", """26
""")

    def test_and10(self):
        self._test("""
puts(false && 42)
""", """false
""")

    def test_and20(self):
        self._test("""
# short circuit

def foo()
  puts(42)
  return false
end

def bar()
  puts(26)
  return true
end

puts(foo() && bar())
""", """42
""")

# vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
