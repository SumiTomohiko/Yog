# -*- coding: utf-8 -*-

from testcase import TestCase

class TestPipeStdin(TestCase):

    def test_pipe0(self):
        self._test(stdout="42", stdin="print(42)")

# vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
