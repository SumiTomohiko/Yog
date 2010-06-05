# -*- coding: utf-8 -*-

from testcase import TestCase

class TestNonlocal(TestCase):

    def test_nonlocal0(self):
        self._test("""
def foo()
  bar = 42
  def baz()
    fuga = \"piyo\"
    def quux()
      nonlocal fuga
      fuga = 3.14
      def hoge()
        nonlocal bar
        bar = 26
      end
      hoge()
    end
    quux()
  end
  baz()
  print(bar) # prints 26
end

foo()
""", "26")

# vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
