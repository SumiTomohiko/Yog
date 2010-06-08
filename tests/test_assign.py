# -*- coding: utf-8 -*-

from testcase import TestCase

class TestAssign(TestCase):

    def test_assign(self):
        self._test("""
foo = 42
puts(foo)
""", "42\n")

    def test_multi_value0(self):
        self._test("""
foo, bar = 42, 26
print(foo)
""", "42")

    def test_multi_value10(self):
        self._test("""
foo, bar = 42, 26
print(bar)
""", "26")

    def test_multi_value20(self):
        self._test("""
*foo = 42
print(foo)
""", "[42]")

    def test_multi_value30(self):
        self._test("""
foo, *bar = 42
print(foo)
""", "42")

    def test_multi_value40(self):
        self._test("""
foo, *bar = 42
print(bar)
""", "[]")

    def test_multi_value50(self):
        self._test("""
foo, *bar = 42, 26
print(foo)
""", "42")

    def test_multi_value60(self):
        self._test("""
foo, *bar = 42, 26
print(bar)
""", "[26]")

    def test_multi_value70(self):
        self._test("""
*foo, bar = 42
print(foo)
""", "[]")

    def test_multi_value80(self):
        self._test("""
*foo, bar = 42
print(bar)
""", "42")

    def test_multi_value90(self):
        self._test("""
*foo, bar = 42, 26
print(foo)
""", "[42]")

    def test_multi_value100(self):
        self._test("""
*foo, bar = 42, 26
print(bar)
""", "26")

    def test_multi_value110(self):
        self._test("""
foo, *bar, baz = 42, 26
print(foo)
""", "42")

    def test_multi_value120(self):
        self._test("""
foo, *bar, baz = 42, 26
print(bar)
""", "[]")

    def test_multi_value130(self):
        self._test("""
foo, *bar, baz = 42, 26
print(baz)
""", "26")

    def test_multi_value140(self):
        self._test("""
foo, *bar, baz = 42, 26, \"foo\"
print(foo)
""", "42")

    def test_multi_value150(self):
        self._test("""
foo, *bar, baz = 42, 26, \"foo\"
print(bar)
""", "[26]")

    def test_multi_value160(self):
        self._test("""
foo, *bar, baz = 42, 26, \"foo\"
print(baz)
""", "foo")

    def test_multi_value170(self):
        self._test("""
foo = 42
bar = 26
foo, bar = bar, foo
print(foo)
""", "26")

    def test_multi_value180(self):
        self._test("""
foo = 42
bar = 26
foo, bar = bar, foo
print(bar)
""", "42")

    def test_UnboundLocalError0(self):
        def test_stderr(stderr):
            assert 0 <= stderr.find("UnboundLocalError: Local variable \"bar\" referenced before assignment")

        self._test("""
def foo()
  print(bar)
  bar = 42
end
foo()
""", stderr=test_stderr)

    def test_UnboundLocalError10(self):
        def test_stderr(stderr):
            assert 0 <= stderr.find("UnboundLocalError: Unbound self referenced")

        self._test("""
def foo()
  print(self)
end
foo()
""", stderr=test_stderr)

# vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
