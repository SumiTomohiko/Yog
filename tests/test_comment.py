# -*- coding: utf-8 -*-

from testcase import TestCase

class TestComment(TestCase):

    def test_comment0(self):
        self._test("""
(: print(42 :)
print(26)
""", "26")

    def test_comment10(self):
        self._test("""
print (: print(42 :) (26)
""", "26")

    def test_comment20(self):
        self._test("""
(:
 : print(42)
 :)
print(26)
""", "26")

    def test_nested0(self):
        self._test("""
(: (: :) print(42) :)
""", "")

    def test_nested10(self):
        self._test("""
(: (: foo() :) print(42) :)
""", "")

# vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
