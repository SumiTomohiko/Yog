# -*- coding: utf-8 -*-

from tests import TestCase

class TestKlass(TestCase):

    def test_method(self):
        self._test("""
class Foo

    def bar()
        puts 42
    end
end

foo = Foo.new()
foo.bar()
""", """42
""")

# vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
