# -*- coding: utf-8 -*-

from os.path import join
from testcase import TestCase

class TestFfi(TestCase):

    def test_load_lib0(self):
        path = join(".", "foo")
        self._test("load_lib(\"%(path)s\")" % locals())

# vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
