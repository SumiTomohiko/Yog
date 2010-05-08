# -*- coding: utf-8 -*-

from os.path import join
from testcase import TestCase
import os

class TestFfi(TestCase):

    def test_load_lib0(self):
        ext = ".so" if os.name != "nt" else ".dll"
        path = join(".", "foo")
        self._test("load_lib(\"%(path)s%(ext)s\")" % locals())

# vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
