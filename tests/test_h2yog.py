# -*- coding: utf-8 -*-

from os import unlink
from testcase import TestCase

class TestH2Yog(TestCase):

    def run_h2yog(self, headers, so):
        src = """from h2yog import h2yog
headers = [%(headers)s].map() do [h]
  next h.to_path()
end
h2yog(\"test_h2yog.yg\", headers, \"%(so)s\") do [path, name]
  next headers.include?(path.basename)
end""" % { "headers": ", ".join([ "\"%s\"" % (header, ) for header in headers]), "so": so }
        path = "run_h2yog.yg"
        unlink(path)
        self.write_source(path, src)
        proc = self.run_command(["--young-heap-size=48M", path])
        self.wait_proc(proc)

    def do_test(self, headers, so, src, expected):
        self.unlink("test_h2yog.yg")
        self.run_h2yog(headers, so)
        self._test(src, expected)

    def test_undef0(self):
        headers = ["test_undef0.h"]
        so = "empty"
        src = """from test_h2yog import FOO
print(FOO)"""
        self.do_test(headers, so, src, "42")

    def test_constant0(self):
        headers = ["test_constant0.h"]
        so = "empty"
        src = """from test_h2yog import FOO
print(FOO)"""
        self.do_test(headers, so, src, "42")

    def do_struct_test(self, header):
        src = """from test_h2yog import Foo
foo = Foo.new()
foo.bar = 42
print(foo.bar)"""
        self.do_test([header], "empty", src, "42")

    def test_struct0(self):
        self.do_struct_test("test_struct0.h")

    def test_struct10(self):
        self.do_struct_test("test_struct10.h")

    def test_struct20(self):
        self.do_struct_test("test_struct20.h")

# vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
