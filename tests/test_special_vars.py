# -*- coding: utf-8 -*-

from os.path import abspath
from testcase import TestCase

class TestSpecialVars(TestCase):

    def test_LINE0(self):
        self._test("puts(__LINE__)", """1
""")

    def test_LINE10(self):
        self._test("""
# comment
puts(__LINE__)
""", """3
""")

    def test_FILE0(self):
        tmpfile = abspath("foo.yog")
        self._test("print(__FILE__)", tmpfile, tmpfile=tmpfile)

# vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
