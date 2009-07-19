# -*- coding: utf-8 -*-

from testcase import TestCase

class TestNil(TestCase):

    def test_literal0(self):
        self._test("""
puts(nil)
""", """nil
""")

# vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
