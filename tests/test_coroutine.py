# -*- coding: utf-8 -*-

from testcase import TestCase

class TestCoroutine(TestCase):

    def test_resume0(self):
        self._test("""
co = Coroutine.new() do
  print(42)
end
co.resume()
""", "42")

    def test_yield0(self):
        self._test("""
co = Coroutine.new() do
  print(42)
  Coroutine.yield()
  print(44)
end
co.resume()
print(43)
co.resume()
print(45)
""", "42434445")

    def test_yield10(self):
        self._test("""
co = Coroutine.new() do
  26.times() do
    print(42)
    Coroutine.yield()
  end
end

26.times() do
  co.resume()
end
""", "4242424242424242424242424242424242424242424242424242")

    def test_return_from_yield0(self):
        self._test("""
co = Coroutine.new() do
  foo, bar = Coroutine.yield()
  print(foo)
end
co.resume()
co.resume(42, 26)
""", "42")

    def test_return_from_yield10(self):
        self._test("""
co = Coroutine.new() do
  foo, bar = Coroutine.yield()
  print(bar)
end
co.resume()
co.resume(42, 26)
""", "26")

    def test_return_from_yield20(self):
        self._test("""
co = Coroutine.new() do
  *foo = Coroutine.yield()
  print(foo)
end
co.resume()
co.resume(42)
""", "[42]")

    def test_return_from_yield30(self):
        self._test("""
co = Coroutine.new() do
  foo, *bar = Coroutine.yield()
  print(foo)
end
co.resume()
co.resume(42)
""", "42")

    def test_return_from_yield40(self):
        self._test("""
co = Coroutine.new() do
  foo, *bar= Coroutine.yield()
  print(bar)
end
co.resume()
co.resume(42)
""", "[]")

    def test_return_from_yield50(self):
        self._test("""
co = Coroutine.new() do
  foo, *bar = Coroutine.yield()
  print(foo)
end
co.resume()
co.resume(42, 26)
""", "42")

    def test_return_from_yield60(self):
        self._test("""
co = Coroutine.new() do
  foo, *bar= Coroutine.yield()
  print(bar)
end
co.resume()
co.resume(42, 26)
""", "[26]")

    def test_return_from_yield70(self):
        self._test("""
co = Coroutine.new() do
  *foo, bar = Coroutine.yield()
  print(foo)
end
co.resume()
co.resume(42)
""", "[]")

    def test_return_from_yield80(self):
        self._test("""
co = Coroutine.new() do
  *foo, bar = Coroutine.yield()
  print(bar)
end
co.resume()
co.resume(42)
""", "42")

    def test_return_from_yield90(self):
        self._test("""
co = Coroutine.new() do
  *foo, bar = Coroutine.yield()
  print(foo)
end
co.resume()
co.resume(42, 26)
""", "[42]")

    def test_return_from_yield100(self):
        self._test("""
co = Coroutine.new() do
  *foo, bar = Coroutine.yield()
  print(baz)
end
co.resume()
co.resume(42, 26)
""", "26")

    def test_return_from_yield110(self):
        self._test("""
co = Coroutine.new() do
  foo, *bar, baz = Coroutine.yield()
  print(foo)
end
co.resume()
co.resume(42, 26)
""", "42")

    def test_return_from_yield120(self):
        self._test("""
co = Coroutine.new() do
  foo, *bar, baz = Coroutine.yield()
  print(bar)
end
co.resume()
co.resume(42, 26)
""", "[]")

    def test_return_from_yield130(self):
        self._test("""
co = Coroutine.new() do
  foo, *bar, baz = Coroutine.yield()
  print(baz)
end
co.resume()
co.resume(42, 26)
""", "26")

    def test_return_from_yield140(self):
        self._test("""
co = Coroutine.new() do
  foo, *bar, baz = Coroutine.yield()
  print(foo)
end
co.resume()
co.resume(42, 26, \"foo\")
""", "42")

    def test_return_from_yield150(self):
        self._test("""
co = Coroutine.new() do
  foo, *bar, baz = Coroutine.yield()
  print(bar)
end
co.resume()
co.resume(42, 26, \"foo\")
""", "[26]")

    def test_return_from_yield160(self):
        self._test("""
co = Coroutine.new() do
  foo, *bar, baz = Coroutine.yield()
  print(baz)
end
co.resume()
co.resume(42, 26, \"foo\")
""", "foo")

    def test_return_from_resume0(self):
        self._test("""
co = Coroutine.new() do
  Coroutine.yield(42, 26)
end
foo, bar = co.resume()
print(foo)
""", "42")

    def test_return_from_resume10(self):
        self._test("""
co = Coroutine.new() do
  Coroutine.yield(42, 26)
end
foo, bar = co.resume()
print(bar)
""", "26")

    def test_return_from_resume20(self):
        self._test("""
co = Coroutine.new() do
  Coroutine.yield(42)
end
*foo = co.resume()
print(foo)
""", "[42]")

    def test_return_from_resume30(self):
        self._test("""
co = Coroutine.new() do
  Coroutine.yield(42)
end
foo, *bar = co.resume()
print(foo)
""", "42")

    def test_return_from_resume40(self):
        self._test("""
co = Coroutine.new() do
  Coroutine.yield(42)
end
foo, *bar = co.resume()
print(bar)
""", "[]")

    def test_return_from_resume50(self):
        self._test("""
co = Coroutine.new() do
  Coroutine.yield(42, 26)
end
foo, *bar = co.resume()
print(foo)
""", "42")

    def test_return_from_resume60(self):
        self._test("""
co = Coroutine.new() do
  Coroutine.yield(42, 26)
end
foo, *bar = co.resume()
print(bar)
""", "[26]")

    def test_return_from_resume70(self):
        self._test("""
co = Coroutine.new() do
  Coroutine.yield(42)
end
*foo, bar = co.resume()
print(foo)
""", "[]")

    def test_return_from_resume80(self):
        self._test("""
co = Coroutine.new() do
  Coroutine.yield(42)
end
*foo, bar = co.resume()
print(bar)
""", "42")

    def test_return_from_resume90(self):
        self._test("""
co = Coroutine.new() do
  Coroutine.yield(42, 26)
end
*foo, bar = co.resume()
print(foo)
""", "[42]")

    def test_return_from_resume100(self):
        self._test("""
co = Coroutine.new() do
  Coroutine.yield(42, 26)
end
*foo, bar = co.resume()
print(bar)
""", "26")

    def test_return_from_resume110(self):
        self._test("""
co = Coroutine.new() do
  Coroutine.yield(42, 26)
end
foo, *bar, baz = co.resume()
print(foo)
""", "42")

    def test_return_from_resume120(self):
        self._test("""
co = Coroutine.new() do
  Coroutine.yield(42, 26)
end
foo, *bar, baz = co.resume()
print(bar)
""", "[]")

    def test_return_from_resume130(self):
        self._test("""
co = Coroutine.new() do
  Coroutine.yield(42, 26)
end
foo, *bar, baz = co.resume()
print(baz)
""", "26")

    def test_return_from_resume140(self):
        self._test("""
co = Coroutine.new() do
  Coroutine.yield(42, 26, \"foo\")
end
foo, *bar, baz = co.resume()
print(foo)
""", "42")

    def test_return_from_resume150(self):
        self._test("""
co = Coroutine.new() do
  Coroutine.yield(42, 26, \"foo\")
end
foo, *bar, baz = co.resume()
print(bar)
""", "[26]")

    def test_return_from_resume160(self):
        self._test("""
co = Coroutine.new() do
  Coroutine.yield(42, 26, \"foo\")
end
foo, *bar, baz = co.resume()
print(baz)
""", "foo")

    def test_arguments_for_first_resume0(self):
        self._test("""
co = Coroutine.new() do [foo]
  print(foo)
end
co.resume(42)
""", "42")

# vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
