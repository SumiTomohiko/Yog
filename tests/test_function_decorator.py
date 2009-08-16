# -*- coding: utf-8 -*-

from testcase import TestCase

class TestFunctionDecorator(TestCase):

    def test_decorator0(self):
        self._test("""
def foo(f)
  print(26)
  return f
end

@foo
def bar()
  print(42)
end

bar()
""", "2642")

# vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
