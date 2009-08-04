# -*- coding: utf-8 -*-

from re import match
from testcase import TestCase
from utils import is_32bit

class TestString(TestCase):

    disabled = is_32bit() is not True

    def test_string_multiply0(self):
        def test_stderr(stderr):
            m = match(r"""Traceback \(most recent call last\):
  File "[^"]+", line 2, in <module>
  File builtin, in String#\*
ArgumentError: argument too big
""", stderr)
            assert m is not None

        self._test("""
puts("xxxxxxxx" * 536870912)
""", stderr=test_stderr)

# vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
