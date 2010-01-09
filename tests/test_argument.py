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
ArgumentError: an unexpected keyword argument "bar"
""", stderr)

        self._test("""
def foo(*args)
end

foo(bar: 42)
""", stderr=test_stderr)

    def test_keyword_argument20(self):
        def test_stderr(stderr):
            self._test_regexp(r"""Traceback \(most recent call last\):
  File "[^"]+", line 5, in <package>
ArgumentError: an unexpected keyword argument "bar"
""", stderr)

        self._test("""
def foo()
end

foo(**{ :bar => 42 })
""", stderr=test_stderr)

    def test_keyword_argument30(self):
        def test_stderr(stderr):
            self._test_regexp(r"""Traceback \(most recent call last\):
  File "[^"]+", line 5, in <package>
ArgumentError: foo\(\) got multiple values for keyword argument "bar"
""", stderr)

        self._test("""
def foo(bar)
end

foo(26, **{ :bar => 42 })
""", stderr=test_stderr)

    def test_keyword_argument40(self):
        def test_stderr(stderr):
            self._test_regexp(r"""Traceback \(most recent call last\):
  File "[^"]+", line 5, in <package>
ArgumentError: foo\(\) got multiple values for keyword argument "bar"
""", stderr)

        self._test("""
def foo(bar)
end

foo(26, bar: 42)
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

    def test_variable_parameter60(self):
        def test_stderr(stderr):
            self._test_regexp(r"""Traceback \(most recent call last\):
  File "[^"]+", line 2, in <package>
TypeError: argument after \* must be an Array, not Fixnum
""", stderr)

        self._test("""
print(*42)
""", stderr=test_stderr)

    def test_variable_parameter70(self):
        def test_stderr(stderr):
            self._test_regexp(r"""Traceback \(most recent call last\):
  File "[^"]+", line 4, in <package>
TypeError: argument after \* must be an Array, not Fixnum
""", stderr)

        self._test("""
def foo()
end
print(*42)
""", stderr=test_stderr)

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

foo(**{ :bar => 42 })
""", """42
""")

    def test_variable_keyword_parameter15(self):
        self._test("""
def foo(n, m)
  print(n + m)
end

foo(**{ :n => 42, :m => 26 })
""", "68")

    def test_variable_keyword_parameter20(self):
        self._test("""
def foo(bar, **baz)
  puts(bar)
end

foo(**{ :bar => 42, :quux => 26})
""", """42
""")

    def test_variable_keyword_parameter30(self):
        self._test("""
def foo(bar, **baz)
  puts(baz[:quux])
end

foo(**{ :bar => 42, :quux => 26})
""", """26
""")

    def test_variable_keyword_parameter40(self):
        def test_stderr(stderr):
            self._test_regexp(r"""Traceback \(most recent call last\):
  File "[^"]+", line 2, in <package>
TypeError: argument after \*\* must be a Dict, not Fixnum
""", stderr)

        self._test("""
print(**42)
""", stderr=test_stderr)

    def test_variable_parameter70(self):
        def test_stderr(stderr):
            self._test_regexp(r"""Traceback \(most recent call last\):
  File "[^"]+", line 4, in <package>
TypeError: argument after \*\* must be a Dict, not Fixnum
""", stderr)

        self._test("""
def foo()
end
print(**42)
""", stderr=test_stderr)

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

    def test_block20(self):
        self._test("""
def foo(&block)
end

foo(&nil) do
end
""", stderr="""SyntaxError: block argument repeated
""")

    def test_block30(self):
        def test_stderr(stderr):
            self._test_regexp(r"""Traceback \(most recent call last\):
  File "[^"]+", line 4, in <package>
ArgumentError: can't accept a block argument
""", stderr)

        self._test("""
def foo()
end
foo() do
end
""", stderr=test_stderr)

    def test_default0(self):
        self._test("""
def foo(bar=42)
  print(bar)
end

foo()
""", "42")

    def test_default10(self):
        self._test("""
def foo(bar=42)
  print(bar)
end

foo(26)
""", "26")

    def test_default20(self):
        self._test("""
def foo(bar, baz=42)
  print(bar)
end

foo(26)
""", "26")

    def test_default30(self):
        self._test("""
def foo(bar, baz=42)
  print(baz)
end

foo(26)
""", "42")

    def test_default40(self):
        self._test("""
def foo(bar, baz=42)
  print(baz)
end

foo(26, \"quux\")
""", "quux")

    def test_default50(self):
        self._test("""
def foo(bar=42, baz=26)
  print(bar)
end

foo()
""", "42")

    def test_default60(self):
        self._test("""
def foo(bar=42, baz=26)
  print(baz)
end

foo()
""", "26")

    def test_default70(self):
        self._test("""
def foo(bar=42, baz=26)
  print(bar)
end

foo(\"quux\")
""", "quux")

    def test_default80(self):
        self._test("""
def foo(bar=42, baz=26)
  print(baz)
end

foo(\"quux\", \"hoge\")
""", "hoge")

    def test_default90(self):
        self._test("""
def foo(bar=42, baz=26)
  print(bar)
end

foo(baz: \"quux\")
""", "42")

    def test_default100(self):
        self._test("""
def foo(bar=42, baz=26)
  print(baz)
end

foo(baz: \"quux\")
""", "quux")

    def test_default110(self):
        self._test("""
def foo(&block=nil)
  print(block)
end

foo()
""", "nil")

    def test_default120(self):
        self._test("""
def foo(&block=nil)
  block()
end

foo() do
  print(42)
end
""", "42")

    def test_default130(self):
        self._test("""
def foo(bar, &block=nil)
  print(bar)
end

foo(42)
""", "42")

    def test_default140(self):
        self._test("""
def foo(bar, &block=nil)
  print(block)
end

foo(42)
""", "nil")

    def test_default150(self):
        self._test("""
def foo(bar=42, &block)
  print(bar)
end

foo() do
end
""", "42")

    def test_default160(self):
        self._test("""
def foo(bar=42, &block)
  block()
end

foo() do
  print(26)
end
""", "26")

    def test_default170(self):
        self._test("""
def foo(bar=42, &block=nil)
  print(bar)
end

foo()
""", "42")

    def test_default180(self):
        self._test("""
def foo(bar=42, &block=nil)
  print(block)
end

foo()
""", "nil")

    def test_default190(self):
        self._test("""
def foo(bar=baz=42)
  print(bar)
end

foo()
""", "42")

    def test_default200(self):
        self._test("""
def foo(bar=baz=42)
  print(baz)
end

foo()
""", "42")

    def test_default210(self):
        self._test("""
def foo(&block=bar=nil)
  print(block)
end

foo()
""", "nil")

    def test_default220(self):
        self._test("""
def foo(&block=bar=nil)
  print(bar)
end

foo()
""", "nil")

# vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
