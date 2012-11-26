# -*- coding: utf-8 -*-

from testcase import TestCase

class TestFunction(TestCase):

    def test_variable_args0(self):
        self._test("""def foo(*args)
  print(args)
end
foo()""", "[]")

    def test_variable_args10(self):
        self._test("""def foo(*args)
  print(args)
end
foo(42)""", "[42]")

    def test_variable_args20(self):
        self._test("""def foo(*args)
  print(args)
end
foo(42, 26)""", "[42, 26]")

    def test_variable_args30(self):
        self._test("""def foo(x, *args)
  print(x)
end
foo(42)""", "42")

    def test_variable_args40(self):
        self._test("""def foo(x, *args)
  print(args)
end
foo(42)""", "[]")

    def test_variable_args50(self):
        self._test("""def foo(x, *args)
  print(args)
end
foo(42, 26)""", "[26]")

    def test_variable_args60(self):
        self._test("""def foo(x, *args)
  print(args)
end
foo(42, 26, \"foo\")""", "[26, \"foo\"]")

    def test_variable_args70(self):
        self._test("""def foo(n=42)
  print(n)
end
foo(*[])""", "42")

    def test_variable_args80(self):
        msg = "foo() requires 1 positional argument(s) (0 given)"
        self._test_exception("""def foo(x)
end
foo(*[])""", "ArgumentError", msg)

    def test_func(self):
        self._test("""
def foo()
    puts(42)
end

foo()
""", "42\n")

    def test_param(self):
        self._test("""
def foo(a)
    puts(a)
end

foo(42)
""", "42\n")

    def test_no_return(self):
        self._test("""
def foo()
end

puts(foo())
""", """nil
""")

    def test_uncallable0(self):
        def test_stderr(stderr):
            self._test_regexp(r"""Traceback \(most recent call last\):
  File "[^"]+", line 2, in <package>
TypeError: Fixnum is not callable
""", stderr)
        self._test("""
42()
""", stderr=test_stderr)

# vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
