# -*- coding: utf-8 -*-

from h2yog_helper import Base

class TestH2Yog(Base):

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

    def test_struct30(self):
        self.do_struct_test("test_struct30.h")

# vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
