# -*- coding: utf-8 -*-

from re import match
from testcase import TestCase

class TestProperty(TestCase):

    def test_property0(self):
        def test_stdout(stdout):
            m = match(r"""<Class [0-9a-fA-Z]+>
""", stdout)
            assert m is not None

        self._test("""
o = Object.new()
puts(o.class)
""", stdout=test_stdout)

    def test_property10(self):
        self._test("""
class Foo
  def get_bar()
    puts(42)
  end

  bar = property(get_bar)
end

foo = Foo.new()
foo.bar
""", """42
""")

    def test_property20(self):
        self._test("""
class Foo
  def get_bar()
    return 42
  end

  bar = property(get_bar)
end

foo = Foo.new()
puts(foo.bar)
""", """42
""")

    def test_property30(self):
        self._test("""
class Foo
  def set_bar(bar)
    puts(42)
  end

  bar = property(nil, set_bar)
end

foo = Foo.new()
foo.bar = 26
""", """42
""")

    def test_property40(self):
        self._test("""
class Foo
  def set_bar(baz)
    self.baz = baz
  end

  def get_bar()
    return self.baz
  end

  bar = property(get_bar, set_bar)
end

foo = Foo.new()
foo.bar = 42
puts(foo.bar)
""", """42
""")

# vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
