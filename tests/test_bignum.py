# -*- coding: utf-8 -*-

from testcase import TestCase

class TestBignum(TestCase):

    def test_negative0(self):
        self._test("""
puts(- 4611686018427387905)
""", """-4611686018427387905
""")

# vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
