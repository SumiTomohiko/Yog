# -*- coding: utf-8 -*-

from re import match
from testcase import TestCase

class TestObject(TestCase):

    def test_to_s0(self):
        def test_stdout(stdout):
            m = match(r"""<Object [0-9a-zA-Z]+>
""", stdout)
            assert m is not None

        self._test("""
puts(Object.new())
""", stdout=test_stdout)

    def test_constructor0(self):
        self._test("""
class Foo
  def init()
    puts(42)
  end
end

foo = Foo.new()
""", """42
""")

    def test_set_attribute0(self):
        self._test("""
o = Object.new()
o.foo = 42
puts(o.foo)
""", """42
""")

    def test_set_attribute10(self):
        def test_stderr(stderr):
            m = match(r"""Traceback \(most recent call last\):
  File "[^"]+", line 2, in <package>
AttributeError: String object has no attribute 'bar'
""", stderr)
            assert m is not None

        self._test("""
"foo".bar = 42
""", stderr=test_stderr)

# vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
