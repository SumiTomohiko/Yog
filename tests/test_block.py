# -*- coding: utf-8 -*-

from tests import TestCase

class TestBlock(TestCase):

    def test_block1(self):
        test._test("1.times() { [n] puts n }", """0
""")

    def test_block2(self):
        test._test("1.times() { puts 42 }", """42
""")

    def test_block3(self):
        test._test("1.times() do [n] puts n end", """0
""")

    def test_block4(self):
        test._test("1.times() do puts 42 end", """42
""")

# vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
