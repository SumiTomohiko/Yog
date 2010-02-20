# -*- coding: utf-8 -*-

from testcase import TestCase

class TestBlock(TestCase):

    def test_block3(self):
        self._test("""
1.times() do [n]
    puts(n)
end
""", """0
""")

    def test_block4(self):
        self._test("""
1.times() do
    puts(42)
end
""", """42
""")

    def test_block7(self):
        self._test("""
n = 42
1.times() do [n]
  puts(n)
end

puts(n)
""", """0
42
""")

    def test_block10(self):
        self._test("""
def main()
  n = 42
  1.times() do [n]
    puts(n)
  end
  puts(n)
end

main()""", """0
42
""")

    def test_return0(self):
        self._test("""
def foo()
  [42].each() do
    return 26
  end
end

print(foo())
""", "26")

    def test_return10(self):
        self._test("""
def foo()
  ["bar"].each() do
    ["baz"].each() do
      return 42
    end
  end
end

print(foo())
""", "42")

    def test_next0(self):
        self._test("""
def foo()
  [42].each() do
    print(26)
    next
    print("bar")
  end
end

foo()
""", "26")

    def test_next10(self):
        self._test("""
def foo()
  [42, 3.1415926535].each() do
    print(26)
    next
    print("bar")
  end
end

foo()
""", "2626")

    def test_self_in_block0(self):
        def test_stdout(stdout):
            self._test_regexp(r"<Foo [0-9a-zA-Z]+>", stdout)

        self._test("""
class Foo
  def bar()
    1.times() do
      print(self)
    end
  end
end

Foo.new().bar()
""", stdout=test_stdout)

# vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
