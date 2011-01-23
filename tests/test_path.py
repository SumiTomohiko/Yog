# -*- coding: utf-8 -*-

from os.path import join
from testcase import TestCase

class TestPath(TestCase):

    def do_property_test(self, path, name, expected):
        src = "print(\"%(path)s\".to_path().%(name)s)" % locals()
        self._test(src, expected)

    def do_basename_test(self, path, expected):
        self.do_property_test(path, "basename", expected)

    def do_dirname_test(self, path, expected):
        self.do_property_test(path, "dirname", expected)

    def test_dirname0(self):
        self.do_dirname_test("foo", ".")

    def test_dirname10(self):
        self.do_dirname_test(join("foo", "bar"), "foo")

    def test_dirname20(self):
        self.do_dirname_test(join("foo", ""), ".")

    def test_dirname30(self):
        self.do_dirname_test(".", ".")

    def test_dirname40(self):
        self.do_dirname_test("..", ".")

    def test_basename0(self):
        self.do_basename_test("foo", "foo")

    def test_basename10(self):
        self.do_basename_test(join("foo", "bar"), "bar")

    def test_basename20(self):
        self.do_basename_test(join("foo", ""), "foo")

    def test_basename30(self):
        self.do_basename_test(".", ".")

    def test_basename40(self):
        self.do_basename_test("..", ".")

# vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4