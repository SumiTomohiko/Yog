# -*- coding: utf-8 -*-

from testcase import TestCase, enumerate_tuples

class TestArray(TestCase):

    for i, a, expected in enumerate_tuples((
            ("[]", "[]"),
            ("[42]", "[42]"),
            ("[42, 26]", "[26, 42]"))):
        exec("""def test_reverse{i}(self):
    self._test(\"print({a}.reverse())\", \"{expected}\")
""".format(i=10 * i, a=a, expected=expected))

    def test_any0(self):
        self._test("print([].any?(&nop))", "false")

    def test_any10(self):
        self._test("""print([42].any?() do |elem|
  next elem == 26
end)""", "false")

    def test_any20(self):
        self._test("""print([42, \"foo\"].any?() do |elem|
  next elem == 26
end)""", "false")

    def test_any30(self):
        self._test("""print([42].any?() do |elem|
  next elem == 42
end)""", "true")

    def test_any40(self):
        self._test("""print([42, 26].any?() do |elem|
  next elem == 26
end)""", "true")

    def test_init0(self):
        self._test("""a = Array.new(42) do |index|
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
a.each() do |n|
  puts(n)
end
""", "")

    def test_each2(self):
        self._test("""
a = [42]
a.each() do |n|
  puts(n)
end
""", """42
""")

    def test_each3(self):
        self._test("""
a = [42, 26]
a.each() do |n|
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
        self._test("""print([].reduce(42) do |init, val|
  next nil
end)""", "42")

    def test_reduce10(self):
        self._test("""print([42].reduce(26) do |init, val|
  next init + val
end)""", "68")

    def test_sort0(self):
        self._test("print([].sort())", "[]")

    def test_sort10(self):
        self._test("print([42].sort())", "[42]")

    def test_sort20(self):
        self._test("print([42, 26].sort())", "[26, 42]")

    def test_sort30(self):
        self._test("print([26, 42].sort())", "[26, 42]")

    def test_sort40(self):
        self._test("""print([26, 42].sort() do |x, y|
  next y <=> x
end)""", "[42, 26]")

    for i, src, expr, expected in enumerate_tuples((
            ("[]", "true", "[[]]"),
            ("[42]", "elem == 42", "[[], []]"),
            ("[42, 26, \"foo\"]", "elem == 42", "[[], [26, \"foo\"]]"),
            ("[42, 26, \"foo\"]", "elem == 26", "[[42], [\"foo\"]]"),
            ("[42, 26, \"foo\"]", "elem == \"foo\"", "[[42, 26], []]"),
            ("[42, 26, \"foo\"]", "true", "[[], [], [], []]"),
            ("[\"foo\", \"quux\", \"bar\", \"quux\", \"baz\"]",
                "elem == \"quux\"",
                "[[\"foo\"], [\"bar\"], [\"baz\"]]"))):
        exec("""def test_split{i}(self):
    self._test(\"\"\"print({src}.split() do |elem|
  next {expr}
end)\"\"\", {expected})
""".format(i=10 * i, src=src, expr=expr, expected=repr(expected)))

    for i, src, expr, max_, expected in enumerate_tuples((
            ("[]", "true", 0, "[[]]"),
            ("[42]", "true", 0, "[[42]]"),
            ("[42]", "true", 1, "[[], []]"),
            ("[42]", "true", 2, "[[], []]"),
            ("[42, 26]", "elem == 26", 1, "[[42], []]"),
            ("[42, 26, 42]", "elem == 26", 1, "[[42], [42]]"),
            ("[42, 26, 42, 26]", "elem == 26", 1, "[[42], [42, 26]]"))):
        exec("""def test_split_max{i}(self):
    self._test(\"\"\"print({src}.split({max}) do |elem|
  next {expr}
end)\"\"\", {expected})
""".format(i=10 * i, src=src, expr=expr, max=max_, expected=repr(expected)))

# vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
