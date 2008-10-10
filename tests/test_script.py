# -*- coding: utf-8 -*-

from tests import TestCase

class TestScript(TestCase):

    def test_empty1(self):
        self._test("", "")

    def test_empty2(self):
        self._test("\n", "")

    def test_empty3(self):
        self._test("\n\n", "")

    def test_empty4(self):
        self._test("""def foo()
end""", "")

    def test_empty5(self):
        self._test("""
def foo()
end""", "")

    def test_empty6(self):
        self._test("""

def foo()
end""", "")

# vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
