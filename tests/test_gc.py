# -*- coding: utf-8 -*-

from testcase import TestCase

class TestGc(TestCase):

    def test_gc(self):
        self._test("""
500.times() do [n]
    o = Object.new()
end
""")

    def test_gc_stress0(self):
        self._test("", options=[ "--gc-stress", "--gc-stress" ])

# vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
