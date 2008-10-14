# -*- coding: utf-8 -*-

from tests import TestCase

class TestInt(TestCase):

    def test_plus_one(self):
        self._test("puts 42 + 1", "43\n")

# vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
