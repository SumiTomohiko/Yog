# -*- coding: utf-8 -*-

from testcase import TestCase

class TestLine(TestCase):

    def test_line1(self):
        self._test("puts(__LINE__)", """1
""")

    def test_line2(self):
        self._test("""
# comment
puts(__LINE__)
""", """3
""")

# vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
