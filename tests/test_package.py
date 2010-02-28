# -*- coding: utf-8 -*-

from testcase import TestCase

class TestPackage(TestCase):

    def test_AttributeError0(self):
        def test_stderr(stderr):
            assert 0 < stderr.find("AttributeError: Package object has no attribute \"foo\"")

        self._test("""
import hq9plus
hq9plus.foo
""", stderr=test_stderr)

# vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
