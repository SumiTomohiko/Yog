# -*- coding: utf-8 -*-

from os.path import join
from unix import TestUnix

class TestUnixPath(TestUnix):

    def do_property_test(self, path, name, expected):
        src = "print(\"%(path)s\".to_path().%(name)s)" % locals()
        self._test(src, expected)

    def do_basename_test(self, path, expected):
        self.do_property_test(path, "basename", expected)

    def do_dirname_test(self, path, expected):
        self.do_property_test(path, "dirname", expected)

    def test_dirname50(self):
        self.do_dirname_test("/", "/")

    def test_basename50(self):
        self.do_basename_test("/", "/")

    def test_get_root0(self):
        self._test("print((Path.get_root() / \"foo\").dirname)", "/")

# vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
