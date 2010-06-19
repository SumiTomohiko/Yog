# -*- coding: utf-8 -*-

from testcase import find_so
from unix import TestUnix

class TestZlib(TestUnix):

    disabled = TestUnix.disabled or (not find_so("o"))
    options = []

    def test_gc0(self):
        self._test("""
import o
""", options=["--gc-stress"])

# vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
