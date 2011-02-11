# -*- coding: utf-8 -*-

from h2yog_helper import Base

class TestH2Yog(Base):

    def do_datatype_test(self, datatype, expected):
        path = "datatype.h"
        self.write_source(path, "struct Foo {{ {0} bar; }};".format(datatype))
        src = """from test_h2yog import Foo
foo = Foo.new()
foo.bar = {0}
print(foo.bar)""".format(expected)
        self.do_test([path], "empty", src, str(expected))

    datatypes = [
        # Tests for int
        ["int", -2147483648],
        ["signed int", -2147483648],
        ["unsigned int", 4294967295],

        # Tests for long
        ["long int", -2147483648],
        ["long", -2147483648],
        ["signed long", -2147483648],
        ["unsigned long", 18446744073709551615],

        # Tests for long long
        ["long long int", -9223372036854775808],
        ["long long", -9223372036854775808],
        ["signed long long", -9223372036854775808],
        ["unsigned long long", 18446744073709551615],

        # Tests for short
        ["short int", -32768],
        ["short", 32768],
        ["signed short", 32768],
        ["unsigned short", 65535],

        # Tests for char
        ["char", -128],
        ["signed char", -128],
        ["unsigned char", 255],

        # Tests for grammar
        ["long signed", -2147483648]]
    for datatype, value in datatypes:
        tmpl = """def test_{0}(self):
    self.do_datatype_test(\"{1}\", {2})"""
        exec tmpl.format(datatype.replace(" ", "_"), datatype, value)

# vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
