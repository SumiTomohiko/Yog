# -*- coding: utf-8 -*-

from re import match
from testcase import TestCase

class TestObject(TestCase):

    def test_to_s0(self):
        def test_stdout(stdout):
            m = match(r"""<Object [0-9a-zA-Z]+>
""", stdout)
            assert m is not None

        self._test("""
puts(Object.new())
""", stdout=test_stdout)

# vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
