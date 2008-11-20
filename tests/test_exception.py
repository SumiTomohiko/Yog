# -*- coding: utf-8 -*-

from re import match
from tests import TestCase

class TestException(TestCase):

    def test_traceback(self):
        def test_stderr(stderr):
            m = match("""Traceback (most recent call last):
  File "[^\"]+", line 1, in <module>
Exception: 42
""", stderr)
            assert m is not None

        self._test("raise Exception.new()", stderr=test_stderr)

# vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
