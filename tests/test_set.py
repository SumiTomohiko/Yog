# -*- coding: utf-8 -*-

from testcase import TestCase

class TestSset(TestCase):

    def test_literal0(self):
        self._test("""
s = { 42 }
print(s.include?(42))
""", "true")

    def test_literal10(self):
        self._test("""
s = { 42, 26 }
print(s.include?(42))
""", "true")

    def test_literal20(self):
        self._test("""
s = { 42, 26 }
print(s.include?(26))
""", "true")

    def test_literal30(self):
        self._test("""
s = { 42, 26 }
print(s.include?(:foo))
""", "false")

    def test_new0(self):
        self._test("""
print(Set.new().size)
""", "0")

    def test_add0(self):
        self._test("""
s = Set.new()
s.add(42)
print(s.include?(42))
""", "true")

# vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
