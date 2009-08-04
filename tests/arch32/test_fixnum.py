# -*- coding: utf-8 -*-

from testcase import TestCase

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
