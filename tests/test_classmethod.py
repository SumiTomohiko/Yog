# -*- coding: utf-8 -*-

from testcase import TestCase

class TestClassMethod(TestCase):

    def test_classmethod0(self):
        self._test("""
class Foo
  def bar()
    puts(42)
  end

  bar = classmethod(bar)
end

Foo.bar()
""", """42
""")

    def test_classmethod10(self):
        self._test("""
class Foo
  def bar()
    puts(42)
  end

  bar = classmethod(bar)
end

Foo.new().bar()
""", """42
""")

    def test_classmethod20(self):
        def test_stdout(stdout):
            self._test_regexp(r"""<Class [0-9a-fA-Z]+>
""", stdout)

        self._test("""
class Foo
  def bar()
    puts(self)
  end

  bar = classmethod(bar)
end

Foo.bar()
""", stdout=test_stdout)

    def test_classmethod30(self):
        def test_stdout(stdout):
            self._test_regexp(r"""<Class [0-9a-fA-Z]+>
""", stdout)

        self._test("""
class Foo
  def bar()
    puts(self)
  end

  bar = classmethod(bar)
end

Foo.new().bar()
""", stdout=test_stdout)

    def test_classmethod40(self):
        def test_stdout(stdout):
            self._test_regexp(r"""<Class [0-9a-fA-Z]+>
""", stdout)

        self._test("""
class Foo
  def bar()
    puts(self)
  end

  bar = classmethod(function: bar)
end

Foo.new().bar()
""", stdout=test_stdout)

# vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
