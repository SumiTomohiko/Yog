# -*- coding: utf-8 -*-

from tests import TestCase

class TestGc(TestCase):

    def test_gc(self):
        self._test("""
500.times() do [n]
    o = Object.new()
end
""")

# vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
