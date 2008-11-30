# -*- coding: utf-8 -*-

from re import match
from tests import TestCase

class TestException(TestCase):

    def test_traceback1(self):
        def test_stderr(stderr):
            m = match(r"""Traceback \(most recent call last\):
  File "[^"]+", line 1, in <module>
Exception: nil
""", stderr)
            assert m is not None

        self._test("raise Exception.new()", stderr=test_stderr)

    def test_traceback2(self):
        def test_stderr(stderr):
            m = match(r"""Traceback \(most recent call last\):
  File "[^"]+", line 6, in <module>
  File "[^"]+", line 3, in foo
Exception: nil
""", stderr)
            assert m is not None

        self._test("""
def foo()
  raise Exception.new()
end

foo()""", stderr=test_stderr)

    def test_traceback3(self):
        def test_stderr(stderr):
            m = match(r"""Traceback \(most recent call last\):
  File "[^"]+", line 9, in <module>
  File "[^"]+", line 4, in Foo#bar
Exception: nil
""", stderr)
            assert m is not None

        self._test("""
class Foo
  def bar()
    raise Exception.new()
  end
end

foo = Foo.new()
foo.bar()""", stderr=test_stderr)

# vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
