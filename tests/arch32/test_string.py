# -*- coding: utf-8 -*-

import pytest
from testcase import TestCase
from utils import is_32bit

@pytest.mark.skipif("is_32bit() is not True")
class TestString(TestCase):

    def test_string_multiply0(self):
        def test_stderr(stderr):
            self._test_regexp(r"""Traceback \(most recent call last\):
  File "[^"]+", line 2, in <package>
ArgumentError: Argument too big
""", stderr)

        self._test("""
puts("xxxxxxxx" * 536870912)
""", stderr=test_stderr)

# vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
