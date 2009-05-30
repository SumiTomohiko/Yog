# -*- coding: utf-8 -*-

from testcase import TestCase

class TestImport(TestCase):

    def test_import1(self):
        self._test("import test_package1", "42\n")

    def test_import2(self):
        self._test("import test_package1, test_package2", "42\n26\n")

# vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
