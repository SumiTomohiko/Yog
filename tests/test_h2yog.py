# -*- coding: utf-8 -*-

from h2yog_helper import Base
from testcase import get_lib_path

def type2base(type):
    return type.replace(" ", "_")

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
datatypes_typedef = datatypes + [
    # Tests for long long
    ["long long int", "longlong"],
    ["long long", "longlong"],
    ["signed long long", "longlong"],
    ["unsigned long long", "ulonglong"]]

class TestH2Yog(Base):

    def test_undef0(self):
        headers = ["test_undef0.h"]
        so = get_lib_path("empty")
        src = """from test_h2yog import FOO
print(FOO)"""
        self.do_test(headers, so, src, "42")

    def test_constant0(self):
        headers = ["test_constant0.h"]
        so = get_lib_path("empty")
        src = """from test_h2yog import FOO
print(FOO)"""
        self.do_test(headers, so, src, "42")

    def do_struct_test(self, header):
        so = get_lib_path("empty")
        src = """from test_h2yog import Foo
foo = Foo.new()
foo.bar = 42
print(foo.bar)"""
        self.do_test([header], so, src, "42")

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

    for type, expected in datatypes_typedef:
        exec """def test_typedef_{0}(self):
    self.do_typedef_test(\"{1}\", \"{2}\")""".format(type2base(type), type, expected)

    def do_func_test(self, base, src, expected):
        self.do_test([base + ".h"], get_lib_path(base), src, expected)

    def test_func0(self):
        base = "test_func0"
        self.do_func_test(base, """from test_h2yog import {0}
{0}()""".format(base), "42")

    def test_func10(self):
        base = "test_func10"
        self.do_func_test(base, """from test_h2yog import {0}
{0}(42, 26)""".format(base), "42")

    for type, _ in datatypes:
        exec """def {0}(self):
    self.do_func_test(\"{0}\", \"\"\"from test_h2yog import {0}
{0}(42)\"\"\", \"42\")""".format("test_argtype_" + type2base(type))

    def test_func20(self):
        base = "test_func20"
        self.do_func_test(base, """from test_h2yog import Foo, {0}
foo = Foo.new()
foo.bar = 42
{0}(foo)""".format(base), "42")

    def test_func30(self):
        base = "test_func30"
        self.do_func_test(base, """from test_h2yog import Foo, {0}
foo = Foo.new()
foo.bar = 42
{0}(foo)""".format(base), "42")

    for type, _ in datatypes:
        exec """def {0}(self):
    self.do_func_test(\"{0}\", \"\"\"from test_h2yog import {0}
print({0}())\"\"\", \"42\")""".format("test_rettype_" + type2base(type))

    def test_func40(self):
        base = "test_func40"
        self.do_func_test(base, """from test_h2yog import {0}
print({0}().bar)""".format(base), "42")

    def test_func50(self):
        base = "test_func50"
        self.do_func_test(base, """from test_h2yog import {0}
print({0}().bar)""".format(base), "42")

    def test_func60(self):
        base = "test_func60"
        self.do_func_test(base, """from test_h2yog import {0}
{0}(42)""".format(base), "42")

if __name__ == "__main__":
    # This part generated headers and codes for .so partially.
    from os.path import dirname, join
    from sys import argv
    d = dirname(argv[0])
    for type, _ in datatypes:
        argtype_base = "test_argtype_" + type2base(type)
        with open(join(d, argtype_base + ".h"), "w") as fp:
            fp.write("void {0}({1});\n".format(argtype_base, type))
        with open(join(d, argtype_base + ".c"), "w") as fp:
            fp.write("""#include <stdio.h>

void
{0}({1} x)
{{
    printf(\"%d\", (int)x);
}}
""".format(argtype_base, type))

        rettype_base = "test_rettype_" + type2base(type)
        with open(join(d, rettype_base + ".h"), "w") as fp:
            fp.write("{0} {1}();".format(type, rettype_base))
        with open(join(d, rettype_base + ".c"), "w") as fp:
            fp.write("""{0}
{1}()
{{
    return 42;
}}
""".format(type, rettype_base))

# vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
