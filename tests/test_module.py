# -*- coding: utf-8 -*-

from testcase import TestCase

class TestModule(TestCase):

    def test_module0(self):
        self._test("""
module Foo
  def bar()
    print(42)
  end
end

class Baz
end
Baz = include_module(Baz, Foo)

baz = Baz.new()
baz.bar()
""", "42")

# vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
