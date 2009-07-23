# -*- coding: utf-8 -*-

from testcase import TestCase

class TestExpr(TestCase):

    def test_expr0(self):
        self._test("""
puts((42 + 26).to_s())
""", """68
""")

# vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
