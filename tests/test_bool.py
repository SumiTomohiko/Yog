# -*- coding: utf-8 -*-

from testcase import TestCase

class TestBool(TestCase):

    def test_to_s0(self):
        self._test("""
print(true)
""", "true")

    def test_to_s10(self):
        self._test("""
print(false)
""", "false")

# vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
