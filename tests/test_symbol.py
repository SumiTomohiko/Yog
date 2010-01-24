# -*- coding: utf-8 -*-

from testcase import TestCase

class TestSymbol(TestCase):

    def test_symbol0(self):
        self._test("""
puts('foo)
""", """foo
""")

# vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
