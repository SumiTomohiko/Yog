# -*- coding: utf-8 -*-

from testcase import TestCase

class TestReturn(TestCase): 

    def test_return1(self):
        self._test("""
def foo()
  return 42
end

puts(foo())
""", """42
""")

    def test_return2(self):
        self._test("""
def foo()
  return 42
end

puts(foo())
puts(43)
""", """42
43
""")

    def test_return3(self):
        self._test("""
def foo()
  return
end

puts(foo())
""", """nil
""")

    def test_finally(self):
        self._test("""
def foo()
  bar = 42
  try
    return bar
  finally
    bar = 43
  end
end

puts(foo())
""", """42
""")

    def test_multi_value0(self):
        self._test("""
def foo()
  return 42, 26
end

bar, baz = foo()
print(bar)
""", "42")

    def test_multi_value10(self):
        self._test("""
def foo()
  return 42, 26
end

bar, baz = foo()
print(baz)
""", "26")

    def test_multi_value20(self):
        self._test("""
def foo()
  return 42
end

*bar = foo()
print(bar)
""", "[42]")

    def test_multi_value30(self):
        self._test("""
def foo()
  return 42
end

bar, *baz = foo()
print(bar)
""", "42")

    def test_multi_value40(self):
        self._test("""
def foo()
  return 42
end

bar, *baz = foo()
print(baz)
""", "[]")

    def test_multi_value50(self):
        self._test("""
def foo()
  return 42, 26
end

bar, *baz = foo()
print(bar)
""", "42")

    def test_multi_value60(self):
        self._test("""
def foo()
  return 42, 26
end

bar, *baz = foo()
print(baz)
""", "[26]")

    def test_multi_value70(self):
        self._test("""
def foo()
  return 42
end

*bar, baz = foo()
print(bar)
""", "[]")

    def test_multi_value80(self):
        self._test("""
def foo()
  return 42
end

*bar, baz = foo()
print(baz)
""", "42")

    def test_multi_value90(self):
        self._test("""
def foo()
  return 42, 26
end

*bar, baz = foo()
print(bar)
""", "[42]")

    def test_multi_value100(self):
        self._test("""
def foo()
  return 42, 26
end

*bar, baz = foo()
print(baz)
""", "26")

    def test_multi_value110(self):
        self._test("""
def foo()
  return 42, 26
end

bar, *baz, quux = foo()
print(bar)
""", "42")

    def test_multi_value120(self):
        self._test("""
def foo()
  return 42, 26
end

bar, *baz, quux = foo()
print(baz)
""", "[]")

    def test_multi_value130(self):
        self._test("""
def foo()
  return 42, 26
end

bar, *baz, quux = foo()
print(quux)
""", "26")

    def test_multi_value140(self):
        self._test("""
def foo()
  return 42, 26, \"foo\"
end

bar, *baz, quux = foo()
print(baz)
""", "[26]")

    def test_multi_value150(self):
        self._test("""
def foo()
  return 42, 26, \"foo\"
end

bar, *baz, quux = foo()
print(baz)
""", "[26]")

    def test_multi_value160(self):
        self._test("""
def foo()
  return 42, 26, \"foo\"
end

bar, *baz, quux = foo()
print(quux)
""", "foo")

    def test_multi_value_from_long_return0(self):
        self._test("""
def foo()
  loop() do
    return 42, 26
  end
end

bar, baz = foo()
print(bar)
""", "42")

    def test_multi_value_from_long_return10(self):
        self._test("""
def foo()
  loop() do
    return 42, 26
  end
end

bar, baz = foo()
print(baz)
""", "26")

    def test_multi_value_from_long_return20(self):
        self._test("""
def foo()
  loop() do
    return 42
  end
end

*bar = foo()
print(bar)
""", "[42]")

    def test_multi_value_from_long_return30(self):
        self._test("""
def foo()
  loop() do
    return 42
  end
end

bar, *baz = foo()
print(bar)
""", "42")

    def test_multi_value_from_long_return40(self):
        self._test("""
def foo()
  loop() do
    return 42
  end
end

bar, *baz = foo()
print(baz)
""", "[]")

    def test_multi_value_from_long_return50(self):
        self._test("""
def foo()
  loop() do
    return 42, 26
  end
end

bar, *baz = foo()
print(bar)
""", "42")

    def test_multi_value_from_long_return60(self):
        self._test("""
def foo()
  loop() do
    return 42, 26
  end
end

bar, *baz = foo()
print(baz)
""", "[26]")

    def test_multi_value_from_long_return70(self):
        self._test("""
def foo()
  loop() do
    return 42
  end
end

*bar, baz = foo()
print(bar)
""", "[]")

    def test_multi_value_from_long_return80(self):
        self._test("""
def foo()
  loop() do
    return 42
  end
end

*bar, baz = foo()
print(baz)
""", "42")

    def test_multi_value_from_long_return90(self):
        self._test("""
def foo()
  loop() do
    return 42, 26
  end
end

*bar, baz = foo()
print(bar)
""", "[42]")

    def test_multi_value_from_long_return100(self):
        self._test("""
def foo()
  loop() do
    return 42, 26
  end
end

*bar, baz = foo()
print(baz)
""", "26")

    def test_multi_value_from_long_return110(self):
        self._test("""
def foo()
  loop() do
    return 42, 26
  end
end

bar, *baz, quux = foo()
print(bar)
""", "42")

    def test_multi_value_from_long_return120(self):
        self._test("""
def foo()
  loop() do
    return 42, 26
  end
end

bar, *baz, quux = foo()
print(baz)
""", "[]")

    def test_multi_value_from_long_return130(self):
        self._test("""
def foo()
  loop() do
    return 42, 26
  end
end

bar, *baz, quux = foo()
print(quux)
""", "26")

    def test_multi_value_from_long_return140(self):
        self._test("""
def foo()
  loop() do
    return 42, 26, \"foo\"
  end
end

bar, *baz, quux = foo()
print(baz)
""", "[26]")

    def test_multi_value_from_long_return150(self):
        self._test("""
def foo()
  loop() do
    return 42, 26, \"foo\"
  end
end

bar, *baz, quux = foo()
print(baz)
""", "[26]")

    def test_multi_value_from_long_return160(self):
        self._test("""
def foo()
  loop() do
    return 42, 26, \"foo\"
  end
end

bar, *baz, quux = foo()
print(quux)
""", "foo")

# vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
