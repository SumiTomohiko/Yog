# -*- coding: utf-8 -*-

from testcase import TestCase

class TestArray(TestCase):

    def test_init0(self):
        self._test("""a = Array.new(42) do [index]
  next index
end
print(a)""","[0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41]")

    def test_init10(self):
        self._test("print(Array.new(42))", "[nil, nil, nil, nil, nil, nil, nil, nil, nil, nil, nil, nil, nil, nil, nil, nil, nil, nil, nil, nil, nil, nil, nil, nil, nil, nil, nil, nil, nil, nil, nil, nil, nil, nil, nil, nil, nil, nil, nil, nil, nil, nil]")

    def test_empty0(self):
        self._test("""
print([].empty?)
""", "true")

    def test_empty10(self):
        self._test("""
print([42].empty?)
""", "false")

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

    def test_literal4(self):
        self._test("print([42, ][0])", "42")

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
bar = { 'baz: foo }
foo.push(bar)
print(foo.to_s())
""", "[{ 'baz: [...] }]")

    def test_to_s70(self):
        self._test("""
foo = ['bar]
print(foo.to_s())
""", "['bar]")

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

    def test_subscript0(self):
        self._test("""
print([42][0])
""", "42")

    def test_subscript10(self):
        self._test("""
print([42][-1])
""", "42")

    def test_subscript20(self):
        self._test("""
print([42, 26][-1])
""", "26")

    def test_subscript30(self):
        def test_stderr(stderr):
            self._test_regexp(r"""Traceback \(most recent call last\):
  File "[^"]+", line 2, in <package>
IndexError: Array index out of range
""", stderr)

        self._test("""
[][0]
""", stderr=test_stderr)

    def test_subscript40(self):
        def test_stderr(stderr):
            self._test_regexp(r"""Traceback \(most recent call last\):
  File "[^"]+", line 2, in <package>
IndexError: Array index out of range
""", stderr)

        self._test("""
[][-1]
""", stderr=test_stderr)

    def test_shift0(self):
        self._test("""
print([42].shift())
""", "42")

    def test_shift10(self):
        self._test("""
foo = [42]
foo.shift()
print(foo)
""", "[]")

    def test_shift20(self):
        def test_stderr(stderr):
            self._test_regexp(r"""Traceback \(most recent call last\):
  File "[^"]+", line 2, in <package>
  File builtin, in Array#shift
IndexError: shift from empty array
""", stderr)

        self._test("""
[].shift()
""", stderr=test_stderr)

    def test_unshift0(self):
        self._test("""
print([42].unshift(26))
""", "[26, 42]")

    def test_get0(self):
        self._test("""
print([].get(0))
""", "nil")

    def test_get10(self):
        self._test("""
print([].get(0, 42))
""", "42")

    def test_get20(self):
        self._test("""
print([42].get(0))
""", "42")

    def test_reduce0(self):
        self._test("""print([].reduce(42) do [init, val]
  next nil
end)""", "42")

    def test_reduce10(self):
        self._test("""print([42].reduce(26) do [init, val]
  next init + val
end)""", "68")

# vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
