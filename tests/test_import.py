# -*- coding: utf-8 -*-

from testcase import TestCase

class TestImport(TestCase):

    def test_import1(self):
        self._test("import test_package", "42\n")

# vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
