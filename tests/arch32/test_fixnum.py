# -*- coding: utf-8 -*-

from platform import machine

import pytest

from testcase import TestCase

@pytest.mark.skipif("machine() != \"i386\"")
class TestFixnum(TestCase):

    def test_left_shift0(self):
        self._test("""
puts(42 << (- 32))
""", """0
""")

    def test_left_shift10(self):
        self._test("""
puts(1 << 32)
""", """4294967296
""")

# vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
