# -*- coding: utf-8 -*-

from testcase import TestCase

class TestBreak(TestCase):

    def test_control_flow0(self):
        self._test("""
def foo()
  [42].each() do
    print(26)
    break
    print("bar")
  end
end

foo()
""", "26")

    def test_control_flow10(self):
        self._test("""
def foo()
  bar = [42].each() do
    break
  end
  print(bar)
end

foo()
""", "nil")

    def test_control_flow20(self):
        self._test("""
def foo()
  bar = [42].each() do
    break 26
  end
  print(bar)
end

foo()
""", "26")

    def test_multi_value0(self):
        self._test("""
foo, bar = [42].each() do
  break 26, \"foo\"
end
print(bar)
""", "foo")

    def test_multi_value10(self):
        self._test("""
foo, bar = [42].each() do
  break 26, \"foo\"
end
print(bar)
""", "foo")

    def test_multi_value20(self):
        self._test("""
*foo = [42].each() do
  break 26
end
print(foo)
""", "[26]")

    def test_multi_value30(self):
        self._test("""
foo, *bar = [42].each() do
  break 26
end
print(foo)
""", "26")

    def test_multi_value40(self):
        self._test("""
foo, *bar = [42].each() do
  break 42
end
print(bar)
""", "[]")

    def test_multi_value50(self):
        self._test("""
foo, *bar = [42].each() do
  break 26, \"foo\"
end
print(foo)
""", "26")

    def test_multi_value60(self):
        self._test("""
foo, *bar = [\"foo\"].each() do
  break 42, 26
end
print(bar)
""", "[26]")

    def test_multi_value70(self):
        self._test("""
*foo, bar = [\"foo\"].each() do
  break 42
end
print(foo)
""", "[]")

    def test_multi_value80(self):
        self._test("""
*foo, bar = [\"foo\"].each() do
  break 42
end
print(bar)
""", "42")

    def test_multi_value90(self):
        self._test("""
*foo, bar = [\"foo\"].each() do
  break 42, 26
end
print(foo)
""", "[42]")

    def test_multi_value100(self):
        self._test("""
*foo, bar = [\"foo\"].each() do
  break 42, 26
end
print(bar)
""", "26")

    def test_multi_value110(self):
        self._test("""
foo, *bar, baz = [\"foo\"].each() do
  break 42, 26
end
print(foo)
""", "42")

    def test_multi_value120(self):
        self._test("""
foo, *bar, baz = [\"foo\"].each() do
  break 42, 26
end
print(bar)
""", "[]")

    def test_multi_value130(self):
        self._test("""
foo, *bar, baz = [\"foo\"].each() do
  break 42, 26
end
print(baz)
""", "26")

    def test_multi_value140(self):
        self._test("""
foo, *bar, baz = [\"foo\"].each() do
  break 42, 26, \"bar\"
end
print(bar)
""", "[26]")

    def test_multi_value150(self):
        self._test("""
foo, *bar, baz = [\"foo\"].each() do
  break 42, 26, \"bar\"
end
print(bar)
""", "[26]")

    def test_multi_value160(self):
        self._test("""
foo, *bar, baz = [\"foo\"].each() do
  break 42, 26, \"bar\"
end
print(baz)
""", "bar")

# vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
