# -*- coding: utf-8 -*-

from testcase import TestCase

class TestPartial(TestCase):

    def test_partial0(self):
        self._test("""
f = partial(print, 42)
f()
""", "42")

    def test_block0(self):
        self._test("""
def foo(&block)
  block()
end

bar = partial(foo) do
  print(42)
end
bar()
""", "42")

    def test_block10(self):
        self._test("""
def foo(&block)
  block()
end

bar = partial(foo)
bar() do
  print(42)
end
""", "42")

    def test_block20(self):
        self._test("""
def foo(&block=nil)
  print(block)
end

bar = partial(foo)
bar()
""", "nil")

# vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
