# -*- coding: utf-8 -*-

from testcase import TestCase

class TestClassDecorator(TestCase):

    def test_include0(self):
        self._test("""
module Foo
  def bar()
    print(42)
  end
end

@include(Foo)
class Baz
end

baz = Baz.new()
baz.bar()
""", "42")

# vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
