# -*- coding: utf-8 -*-

from testcase import TestCase

class TestArray(TestCase):

    def test_literal1(self):
        self._test("""
a = [42]
puts(a[0])
""", """42
""")

    def test_literal2(self):
        self._test("""
a = [42, 26]
puts(a[1])
""", """26
""")

    def test_literal3(self):
        self._test("""
a = []
""", "")

    def test_lshift1(self):
        self._test("""
a = []
a << 42
puts(a[0])
""", """42
""")

    def test_each1(self):
        self._test("""
a = []
a.each() do [n]
  puts(n)
end
""", "")

    def test_each2(self):
        self._test("""
a = [42]
a.each() do [n]
  puts(n)
end
""", """42
""")

    def test_each3(self):
        self._test("""
a = [42, 26]
a.each() do [n]
  puts(n)
end
""", """42
26
""")

    def test_add0(self):
        self._test("""
print(([] + []).size)
""", "0")

    def test_add10(self):
        self._test("""
print(([42] + [26]).size)
""", "2")

    def test_add20(self):
        self._test("""
print(([42] + [26])[0])
""", "42")

    def test_add30(self):
        self._test("""
print(([42] + [26])[1])
""", "26")

    def test_to_s0(self):
        self._test("""
print([].to_s())
""", "[]")

    def test_to_s10(self):
        self._test("""
print([42].to_s())
""", "[42]")

    def test_to_s20(self):
        self._test("""
print([42, 26].to_s())
""", "[42, 26]")

    def test_to_s30(self):
        self._test("""
foo = []
foo.push(foo)
print(foo.to_s())
""", "[[...]]")

    def test_to_s40(self):
        self._test("""
foo = [42]
foo.push(foo)
print(foo.to_s())
""", "[42, [...]]")

    def test_to_s50(self):
        self._test("""
foo = []
foo.push(foo)
foo.push(42)
print(foo.to_s())
""", "[[...], 42]")

    def test_to_s60(self):
        self._test("""
foo = []
bar = { :baz => foo }
foo.push(bar)
print(foo.to_s())
""", "[{ :baz => [...] }]")

    def test_to_s70(self):
        self._test("""
foo = [:bar]
print(foo.to_s())
""", "[:bar]")

    def test_push0(self):
        self._test("""
foo = []
foo.push(42)
print(foo)
""", "[42]")

    def test_include0(self):
        self._test("""
print([42, 26].include?(42))
""", "true")

    def test_include10(self):
        self._test("""
print([42, 26].include?("foo"))
""", "false")

    def test_join0(self):
        self._test("""
print(["foo"].join(":"))
""", "foo")

    def test_join10(self):
        self._test("""
print([].join(":"))
""", "")

    def test_join20(self):
        self._test("""
print(["foo", "bar"].join(":"))
""", "foo:bar")

    def test_assign_subscript0(self):
        self._test("""
foo = [42]
foo[0] = 26
print(foo[0])
""", "26")

    def test_IndexError0(self):
        def test_stderr(stderr):
            self._test_regexp(r"""Traceback \(most recent call last\):
  File "[^"]+", line 2, in <package>
  File builtin, in Array#\[\]
IndexError: array index out of range
""", stderr)

        self._test("""
[][0]
""", stderr=test_stderr)

# vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
