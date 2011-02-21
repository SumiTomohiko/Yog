# -*- coding: utf-8 -*-

from h2yog_helper import Base
from testcase import get_lib_path

def type2base(type):
    return type.replace(" ", "_").replace("*", "p")

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
    ["long signed", "long"],

    # Test(s) for void*
    ["void*", "[\'pointer, \'void]"]]
datatypes_typedef = datatypes + [
    # Tests for long long
    ["long long int", "longlong"],
    ["long long", "longlong"],
    ["signed long long", "longlong"],
    ["unsigned long long", "ulonglong"]]

class TestH2Yog(Base):

    def do_simple_print_test(self, header, expected, name="FOO"):
        self.do_test2(header, """from test_h2yog import {0}
print({0})""".format(name), expected)

    def do_test_enum_type(self, header):
        self.do_simple_print_test(header, "int", "Foo")

    def test_enum0(self):
        self.do_test_enum_type("enum Foo { BAR };")

    def test_enum10(self):
        self.do_test_enum_type("typedef enum foo_t { BAR } Foo;")

    def test_enum20(self):
        self.do_test_enum_type("typedef enum { BAR } Foo;")

    def test_enum30(self):
        self.do_simple_print_test("enum { FOO };", "0")

    def test_enum40(self):
        self.do_simple_print_test("enum { BAR, FOO };", "1")

    def test_enum50(self):
        self.do_simple_print_test("enum { BAR = 42, FOO };", "43")

    def test_enum60(self):
        self.do_simple_print_test("enum { BAR, FOO = 42 };", "42")

    def test_enum70(self):
        self.do_simple_print_test("""enum { BAR };
enum { FOO = BAR };""", "0")

    def test_undef0(self):
        headers = ["test_undef0.h"]
        so = get_lib_path("empty")
        src = """from test_h2yog import FOO
print(FOO)"""
        self.do_test(headers, so, src, "42")

    def test_constant0(self):
        self.do_simple_print_test("#define FOO 42", "42")

    def test_constant10(self):
        self.do_simple_print_test("#define FOO 0x2a", "42")

    def test_expression0(self):
        self.do_simple_print_test("#define FOO 1 << 2", "4")

    def test_expression10(self):
        self.do_simple_print_test("#define FOO (42)", "42")

    def test_expression20(self):
        self.do_simple_print_test("#define FOO 42 | 26", "58")

    def test_expression30(self):
        self.do_simple_print_test("""typedef unsigned int size_t;
#define FOO (size_t)-42""", "-42")

    def test_expression40(self):
        self.do_simple_print_test("#define FOO ~42", "-43")

    def do_struct_test(self, header):
        so = get_lib_path("empty")
        src = """from test_h2yog import Foo
foo = Foo.new()
foo.bar = 42
print(foo.bar)"""
        self.do_test([header], so, src, "42")

    do_union_test = do_struct_test

    def test_union0(self):
        self.do_union_test("test_union0.h")

    def test_union10(self):
        self.do_union_test("test_union10.h")

    def test_union20(self):
        self.do_union_test("test_union20.h")

    def test_union30(self):
        self.do_union_test("test_union30.h")

    def test_union40(self):
        self.do_simple_print_test("union Foo;", "void", "Foo")

    def test_union50(self):
        self.do_simple_print_test("""union Foo;
typedef union Foo Foo;""", "void", "Foo")

    def test_union60(self):
        self.do_simple_print_test("""union Foo_t;
typedef union Foo_t Foo;""", "void", "Foo")

    def test_union70(self):
        base = "test_union70"
        src = """from test_h2yog import Foo, {0}
foo = Foo.new()
foo.bar.baz = 42
{0}(foo)""".format(base)
        self.do_test([base + ".h"], get_lib_path(base), src, "42")

    def test_struct0(self):
        self.do_struct_test("test_struct0.h")

    def test_struct10(self):
        self.do_struct_test("test_struct10.h")

    def test_struct20(self):
        self.do_struct_test("test_struct20.h")

    def test_struct30(self):
        self.do_struct_test("test_struct30.h")

    def test_struct40(self):
        self.do_simple_print_test("struct Foo;", "void", "Foo")

    def test_struct50(self):
        self.do_simple_print_test("""struct Foo;
typedef struct Foo Foo;""", "void", "Foo")

    def test_struct60(self):
        self.do_simple_print_test("""struct Foo_t;
typedef struct Foo_t Foo;""", "void", "Foo")

    def test_struct70(self):
        base = "test_struct70"
        src = """from test_h2yog import Foo, {0}
foo = Foo.new()
foo.bar.baz = 42
{0}(foo)""".format(base)
        self.do_test([base + ".h"], get_lib_path(base), src, "42")

    def do_typedef_test(self, type, expected):
        self.do_simple_print_test("typedef {0} Foo;".format(type), expected, "Foo")

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

    def test_func70(self):
        self.do_func_test("test_func70", """from test_h2yog import foo
foo()""", "42")

    def do_simple_array_test(self, base):
        src = """from test_h2yog import Foo
foo = Foo.new()
foo.bar[0] = 42
print(foo.bar[0])"""
        self.do_test([base + ".h"], get_lib_path("empty"), src, "42")

    for base in ["test_array0", "test_array10", "test_array20"]:
        exec """def {0}(self):
    self.do_simple_array_test(\"{0}\")""".format(base)

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
    return ({0})42;
}}
""".format(type, rettype_base))

# vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
