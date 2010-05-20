# -*- coding: utf-8 -*-

from os.path import join
from testcase import TestCase
import os

class TestFfi(TestCase):

    def get_lib_path(self):
        return join(".", "foo" + ".so" if os.name == "posix" else ".dll")

    def test_load_lib0(self):
        path = self.get_lib_path()
        self._test("load_lib(\"%(path)s\")" % locals())

    def test_find_function0(self):
        path = self.get_lib_path()
        self._test("""
lib = load_lib(\"%(path)s\")
f = lib.load_func(\"foo\")
f()
""" % locals(), "42")

# vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
