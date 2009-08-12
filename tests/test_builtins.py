# -*- coding: utf-8 -*-

from testcase import TestCase

class TestPuts(TestCase):

    def test_int(self):
        self._test("""
puts(42)
""", "42\n")

    def test_print0(self):
        self._test("""
print()
""", "")

    def test_print10(self):
        self._test("""
print("foo")
""", "foo")

# vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
