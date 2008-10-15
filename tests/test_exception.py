# -*- coding: utf-8 -*-

from tests import TestCase

class TestException(TestCase):

    def test_except(self):
        self._test("""
try
    raise 0
except
    puts 42
end""", "42\n")

# vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
