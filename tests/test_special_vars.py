# -*- coding: utf-8 -*-

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
        self._test("""
print(__FILE__)
""", "foo.yg", tmpfile="foo.yg")

# vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
