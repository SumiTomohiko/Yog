# -*- coding: utf-8 -*-

from tests import TestCase

class TestPuts(TestCase):

    def test_int(self):
        self._test("puts 42", "42\n")

# vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
