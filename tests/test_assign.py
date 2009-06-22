# -*- coding: utf-8 -*-

from testcase import TestCase

class TestAssign(TestCase):

    def test_assign(self):
        self._test("""
foo = 42
puts(foo)
""", "42\n")

# vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
