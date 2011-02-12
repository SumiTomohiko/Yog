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

    def do_typedef_test(self, type, expected):
        header = "typedef {0} Foo;".format(type)
        src = """from test_h2yog import Foo
print(Foo)"""
        self.do_test2(header, src, expected)

    datatypes = [
        # Tests for int
        ["int", "int"],
        ["signed int", "int"],
        ["unsigned int", "uint"],

        # Tests for long
        ["long int", "long"],
        ["long", "long"],
        ["signed long", "long"],
        ["unsigned long", "ulong"],

        # Tests for long long
        ["long long int", "longlong"],
        ["long long", "longlong"],
        ["signed long long", "longlong"],
        ["unsigned long long", "ulonglong"],

        # Tests for short
        ["short int", "short"],
        ["short", "short"],
        ["signed short", "short"],
        ["unsigned short", "ushort"],

        # Tests for char
        ["char", "char"],
        ["signed char", "char"],
        ["unsigned char", "uchar"],

        # Tests for grammar
        ["long signed", "long"]]
    for type, expected in datatypes:
        exec """def test_typedef_{0}(self):
    self.do_typedef_test(\"{1}\", \"{2}\")""".format(type.replace(" ", "_"), type, expected)

# vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
