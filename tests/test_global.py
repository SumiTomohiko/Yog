# -*- coding: utf-8 -*-

from testcase import TestCase

class TestGlobal(TestCase):

    def test_global1(self):
        self._test("""
def foo()
  puts 42
end

def bar()
  foo()
end

bar()""", """42
""")

    def test_global2(self):
        self._test("""
foo = 42

def bar()
  puts foo
end

bar()""", """42
""")

    def test_global3(self):
        self._test("""
foo = 42

def bar()
  nonlocal foo
  foo = 43
end

bar()

puts foo""", """43
""")

# vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
