# -*- coding: utf-8 -*-

from tests import TestCase

class TestInt(TestCase):

    def test_plus_one(self):
        self._test("puts 42 + 1", "43\n")

    def test_compare1(self):
        self._test("puts 0 < 42", "true\n")

    def test_compare2(self):
        self._test("puts 42 < 0", "false\n")

    def test_times(self):
        self._test("""
10.times() { (n) 
    puts n
}
""", """0
1
2
3
4
5
6
7
8
9
""")

# vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
