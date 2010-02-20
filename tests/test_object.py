# -*- coding: utf-8 -*-

from testcase import TestCase

class TestObject(TestCase):

    def test_to_s0(self):
        def test_stdout(stdout):
            self._test_regexp(r"""<Object [0-9a-zA-Z]+>
""", stdout)

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
            self._test_regexp(r"""Traceback \(most recent call last\):
  File "[^"]+", line 2, in <package>
AttributeError: String object has no attribute "bar"
""", stderr)

        self._test("""
"foo".bar = 42
""", stderr=test_stderr)

    def test_kind_of0(self):
        self._test("""
print(Object.new().kind_of?(Object))
""", "true")

    def test_kind_of10(self):
        self._test("""
print(42.kind_of?(Object))
""", "true")

    def test_kind_of20(self):
        self._test("""
class Foo
end
print(42.kind_of?(Foo))
""", "false")

# vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
