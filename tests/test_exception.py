# -*- coding: utf-8 -*-

from re import match
from testcase import TestCase

class TestException(TestCase):

    def test_traceback1(self):
        def test_stderr(stderr):
            m = match(r"""Traceback \(most recent call last\):
  File "[^"]+", line 1, in <package>
Exception: nil
""", stderr)
            assert m is not None

        self._test("raise(Exception.new())", stderr=test_stderr)

    def test_traceback2(self):
        def test_stderr(stderr):
            m = match(r"""Traceback \(most recent call last\):
  File "[^"]+", line 6, in <package>
  File "[^"]+", line 3, in foo
Exception: nil
""", stderr)
            assert m is not None

        self._test("""
def foo()
  raise(Exception.new())
end

foo()""", stderr=test_stderr)

    def test_traceback3(self):
        def test_stderr(stderr):
            m = match(r"""Traceback \(most recent call last\):
  File "[^"]+", line 9, in <package>
  File "[^"]+", line 4, in Foo#bar
Exception: nil
""", stderr)
            assert m is not None

        self._test("""
class Foo
  def bar()
    raise(Exception.new())
  end
end

foo = Foo.new()
foo.bar()""", stderr=test_stderr)

    def test_traceback_block1(self):
        def test_stderr(stderr):
            m = match(r"""Traceback \(most recent call last\):
  File "[^"]+", line 2, in <package>
  File builtin, in Fixnum#times
  File "[^"]+", line 3, in <block>
Exception: nil
""", stderr)
            assert m is not None

        self._test("""
1.times() do [n]
  raise(Exception.new())
end""", stderr=test_stderr)

    def test_traceback_block2(self):
        def test_stderr(stderr):
            m = match(r"""Traceback \(most recent call last\):
  File "[^"]+", line 8, in <package>
  File "[^"]+", line 3, in foo
  File builtin, in Fixnum#times
  File "[^"]+", line 4, in <block>
Exception: nil
""", stderr)
            assert m is not None

        self._test("""
def foo()
  1.times() do [n]
    raise(Exception.new())
  end
end

foo()""", stderr=test_stderr)

    def test_traceback_block3(self):
        def test_stderr(stderr):
            m = match(r"""Traceback \(most recent call last\):
  File "[^"]+", line 11, in <package>
  File "[^"]+", line 4, in Foo#bar
  File builtin, in Fixnum#times
  File "[^"]+", line 5, in <block>
Exception: nil
""", stderr)
            assert m is not None

        self._test("""
class Foo
  def bar()
    1.times() do [n]
      raise(Exception.new())
    end
  end
end

foo = Foo.new()
foo.bar()""", stderr=test_stderr)

    def test_traceback_klass(self):
        def test_stderr(stderr):
            m = match(r"""Traceback \(most recent call last\):
  File "[^"]+", line 2, in <package>
  File "[^"]+", line 3, in <class Foo>
Exception: nil
""", stderr)
            assert m is not None

        self._test("""
class Foo
  raise(Exception.new())
end""", stderr=test_stderr)

# vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
