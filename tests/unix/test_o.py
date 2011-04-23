# -*- coding: utf-8 -*-

import pytest
from testcase import find_so
from unix import TestUnix

@pytest.mark.skipif("not find_so(\"o\")")
class TestZlib(TestUnix):

    options = []

    def test_gc0(self):
        self._test("""
import o
""", options=["--gc-stress"])

# vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
