# -*- coding: utf-8 -*-

from testcase import TestCase

class TestSuper(TestCase):

    def test_super0(self):
        self._test("""
class Foo
  def bar()
    print(42)
  end
end

class Baz > Foo
  def bar()
    super()
  end
end

Baz.new().bar()
""", "42")

# vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
