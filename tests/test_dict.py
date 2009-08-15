# -*- coding: utf-8 -*-

from re import match
from testcase import TestCase

class TestDict(TestCase):

    def test_literal0(self):
        self._test("""
d = {}
puts(d.size)
""", """0
""")

    def test_literal5(self):
        self._test("""
d = { 42 => 26 }
puts(d[42])
""", """26
""")

    def test_literal10(self):
        self._test("""
d = { 42 => 26, }
puts(d[42])
""", """26
""")

    def test_literal20(self):
        self._test("""
d = { 42 => 26, "foo" => "bar" }
puts(d[42])
""", """26
""")

    def test_literal30(self):
        self._test("""
d = { foo: "bar" }
puts(d[:foo])
""", """bar
""")

    def test_dict0(self):
        self._test("""
def foo(d)
  puts(d[42])
end

d = Dict.new()
d[42] = 26
foo(d)
""", """26
""")

    def test_dict10(self):
        self._test("""
def foo(d)
  puts(d[4611686018427387904])
end

d = Dict.new()
d[4611686018427387904] = 42
foo(d)
""", """42
""")

    def test_dict20(self):
        self._test("""
def foo(d)
  puts(d["foo"])
end

d = Dict.new()
d["foo"] = 42
foo(d)
""", """42
""")

    def test_KeyError0(self):
        def test_stderr(stderr):
            m = match(r"""Traceback \(most recent call last\):
  File "[^"]+", line 3, in <package>
  File builtin, in Dict#\[\]
KeyError: .*
""", stderr)
            assert m is not None

        self._test("""
d = Dict.new()
puts(d["foo"])
""", stderr=test_stderr)

    def test_add0(self):
        self._test("""
print(({} + {}).size)
""", "0")

    def test_add10(self):
        self._test("""
print(({ foo: 42 } + { bar: 26 }).size)
""", "2")

    def test_add20(self):
        self._test("""
print(({ foo: 42 } + { bar: 26 })[:foo])
""", "42")

    def test_add30(self):
        self._test("""
print(({ foo: 42 } + { bar: 26 })[:bar])
""", "26")

# vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
