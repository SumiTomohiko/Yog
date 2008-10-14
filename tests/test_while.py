# -*- coding: utf-8 -*-

from tests import TestCase

class TestWhile(TestCase):

    def test_while1(self):
        self._test("""
i = 0
while i < 1
    puts i
    i = i + 1
end""", """0
""")

    def test_while2(self):
        self._test("""
i = 0
while i < 2
    puts i
    i = i + 1
end""", """0
1
""")

    def test_while3(self):
        self._test("""
i = 0
while i < 0
    puts i
    i = i + 1
end""", "")

# vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
