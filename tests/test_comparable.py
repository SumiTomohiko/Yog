# -*- coding: utf-8 -*-

from testcase import TestCase

class TestComparable(TestCase):

    def test_equal0(self):
        self._test("""
print(42.==(26))
""", "false")

    def test_equal10(self):
        self._test("""
print(42.==(42))
""", "true")

    def test_not_equal0(self):
        self._test("""
print(42.!=(26))
""", "true")

    def test_not_equal10(self):
        self._test("""
print(42.!=(42))
""", "false")

    def test_less0(self):
        self._test("""
print(42.<(42))
""", "false")

    def test_less10(self):
        self._test("""
print(26.<(42))
""", "true")

    def test_less_equal0(self):
        self._test("""
print(26.<=(42))
""", "true")

    def test_less_equal10(self):
        self._test("""
print(42.<=(42))
""", "true")

    def test_less_equal20(self):
        self._test("""
print(42.<=(26))
""", "false")

    def test_greater0(self):
        self._test("""
print(42.>(42))
""", "false")

    def test_greater10(self):
        self._test("""
print(42.>(26))
""", "true")

    def test_greater_equal0(self):
        self._test("""
print(42.>=(26))
""", "true")

    def test_greater_equal0(self):
        self._test("""
print(42.>=(42))
""", "true")

    def test_greater_equal0(self):
        self._test("""
print(26.>=(42))
""", "false")

# vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
