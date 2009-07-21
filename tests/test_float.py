# -*- coding: utf-8 -*-

from testcase import TestCase

class TestFloat(TestCase):

    def test_float0(self):
        self._test("""
puts(3.1415926535)
""", """3.14159
""")

    def test_negative0(self):
        self._test("""
puts(- 3.1415926535)
""", """-3.14159
""")

# vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4