# -*- coding: utf-8 -*-

from testcase import TestCase

class TestArgument(TestCase):

    def test_variable_argument0(self):
        self._test("""
def foo(*args)
  puts(args.size)
end

foo()
""", """0
""")

    def test_variable_argument10(self):
        self._test("""
def foo(*args)
  puts(args.size)
end

foo(42)
""", """1
""")

    def test_variable_argument20(self):
        self._test("""
def foo(*args)
  puts(args[0])
end

foo(42)
""", """42
""")

    def test_variable_argument30(self):
        self._test("""
def foo(*args)
  puts(args[0])
end

foo(42, 26)
""", """42
""")

    def test_variable_argument40(self):
        self._test("""
def foo(*args)
  puts(args[1])
end

foo(42, 26)
""", """26
""")

    def test_variable_argument50(self):
        self._test("""
def foo(bar, *args)
  puts(bar)
end

foo(42)
""", """42
""")

    def test_variable_argument60(self):
        self._test("""
def foo(bar, *args)
  puts(args.size)
end

foo(42)
""", """0
""")

    def test_variable_argument70(self):
        self._test("""
def foo(bar, *args)
  puts(args.size)
end

foo(42, 26)
""", """1
""")

    def test_variable_argument80(self):
        self._test("""
def foo(bar, *args)
  puts(args[0])
end

foo(42, 26)
""", """26
""")

    def test_keyword_argument0(self):
        self._test("""
def foo(bar)
  puts(bar)
end

foo(bar: 42)
""", """42
""")

    def test_keyword_argument10(self):
        def test_stderr(stderr):
            self._test_regexp(r"""Traceback \(most recent call last\):
  File "[^"]+", line 5, in <package>
TypeError: an unexpected keyword argument 'bar'
""", stderr)

        self._test("""
def foo(*args)
end

foo(bar: 42)
""", stderr=test_stderr)

    def test_variable_keyword_argument0(self):
        self._test("""
def foo(**bar)
  puts(bar.size)
end

foo()
""", """0
""")

    def test_variable_keyword_argument10(self):
        self._test("""
def foo(**bar)
  puts(bar[:baz])
end

foo(baz: 42)
""", """42
""")

    def test_variable_parameter0(self):
        self._test("""
def foo()
  puts(42)
end

bar = []
foo(*bar)
""", """42
""")

    def test_variable_parameter10(self):
        self._test("""
def foo(bar)
  puts(bar)
end

baz = [42]
foo(*baz)
""", """42
""")

    def test_variable_parameter20(self):
        self._test("""
def foo(bar, baz)
  puts(bar)
end

quux = [42, 26]
foo(*quux)
""", """42
""")

    def test_variable_parameter30(self):
        self._test("""
def foo(bar, baz)
  puts(baz)
end

quux = [42, 26]
foo(*quux)
""", """26
""")

    def test_variable_parameter40(self):
        self._test("""
def foo(bar, baz)
  puts(bar)
end

quux = [42]
foo(26, *quux)
""", """26
""")

    def test_variable_parameter50(self):
        self._test("""
def foo(bar, baz)
  puts(baz)
end

quux = [42]
foo(26, *quux)
""", """42
""")

    def test_variable_keyword_parameter0(self):
        self._test("""
def foo()
  puts(42)
end

foo(**{})
""", """42
""")

    def test_variable_keyword_parameter10(self):
        self._test("""
def foo(bar)
  puts(bar)
end

foo(**{ bar: 42 })
""", """42
""")

    def test_variable_keyword_parameter15(self):
        self._test("""
def foo(n, m)
  print(n + m)
end

foo(**{ n: 42, m: 26 })
""", "68")

    def test_variable_keyword_parameter20(self):
        self._test("""
def foo(bar, **baz)
  puts(bar)
end

foo(**{ bar: 42, quux: 26})
""", """42
""")

    def test_variable_keyword_parameter30(self):
        self._test("""
def foo(bar, **baz)
  puts(baz[:quux])
end

foo(**{ bar: 42, quux: 26})
""", """26
""")

    def test_block0(self):
        self._test("""
def foo(&block)
  block()
end

foo() do
  print(42)
end
""", "42")

    def test_block10(self):
        self._test("""
def foo(&block)
  block(42)
end

foo() do [n]
  print(n)
end
""", "42")

# vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
