# -*- coding: utf-8 -*-

from re import match
from testcase import TestCase

class TestPuts(TestCase):

    def test_int(self):
        self._test("""
puts(42)
""", "42\n")

    def test_print0(self):
        self._test("""
print()
""", "")

    def test_print10(self):
        self._test("""
print("foo")
""", "foo")

    def test_ARGV0(self):
        self._test("""
print(ARGV[0])
""", "foo.yg", tmpfile="foo.yg")

    def test_ARGV1(self):
        self._test("""
print(ARGV[1])
""", "foo", yog_option=["foo"])

    def test_ARGV2(self):
        self._test("""
print(ARGV[2])
""", "bar", yog_option=["foo", "bar"])

    def test_partial0(self):
        self._test("""
f = partial(print, 42)
f()
""", "42")

    def test_bind0(self):
        def test_stdout(stdout):
            m = match(r"<Foo [0-9A-Za-z]+", stdout)
            assert m is not None

        self._test("""
class Foo
  def bar()
    @bind(self)
    def baz()
      print(self)
    end

    return baz
  end
end

foo = Foo.new()
bar = foo.bar()
bar()
""", stdout=test_stdout)

# vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
