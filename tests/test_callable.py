# -*- coding: utf-8 -*-

from testcase import TestCase

class TestCallable(TestCase):

    def test_reverse_args0(self):
        self._test("""def foo(*args)
  print(*args)
end
foo.reverse_args()()""", "")

    def test_reverse_args10(self):
        self._test("""def foo(*args)
  print(*args)
end
foo.reverse_args()(42)""", "42")

    def test_reverse_args20(self):
        self._test("""def foo(*args)
  print(*args)
end
foo.reverse_args()(42, 26)""", "2642")

    def test_partial0(self):
        self._test("""f = print.partial(42)
f()""", "42")

    def test_block0(self):
        self._test("""def foo(&block)
  block()
end

bar = foo.partial() do
  print(42)
end
bar()""", "42")

    def test_block10(self):
        self._test("""def foo(&block)
  block()
end

bar = foo.partial()
bar() do
  print(42)
end""", "42")

    def test_block20(self):
        self._test("""def foo(&block=nil)
  print(block)
end

bar = foo.partial()
bar()""", "nil")

# vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
