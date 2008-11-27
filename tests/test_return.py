# -*- coding: utf-8 -*-

from tests import TestCase

class TestReturn(TestCase): 

    def test_return1(self):
        self._test("""
def foo()
  return 42
end

puts foo()
""", """42
""")

    def test_return2(self):
        self._test("""
def foo()
  return 42
end

puts foo()
puts 43
""", """42
43
""")

    def test_return3(self):
        self._test("""
def foo()
  return
end

puts foo()
""", """nil
""")

# vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
