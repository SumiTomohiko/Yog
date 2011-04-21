# -*- coding: utf-8 -*-

from h2yog_helper import Base

class TestH2Yog(Base):

    def do_datatype_test(self, datatype, expected):
        header = "struct Foo {{ {0} bar; }};".format(datatype)
        src = """from test_h2yog import Foo
foo = Foo.new()
foo.bar = {0}
print(foo.bar)""".format(expected)
        self.do_test2(header, src, str(expected))

    datatypes = [
        # Tests for int
        ["int", -2147483648],
        ["signed int", -2147483648],
        ["unsigned int", 4294967295],

        # Tests for long
        ["long int", -2147483648],
        ["long", -2147483648],
        ["signed long", -2147483648],
        ["unsigned long", 4294967295],

        # Tests for short
        ["short int", -32768],
        ["short", 32767],
        ["signed short", 32767],
        ["unsigned short", 65535],

        # Tests for char
        ["char", -128],
        ["signed char", -128],
        ["unsigned char", 255],

        # Tests for grammar
        ["long signed", -2147483648]]
    for datatype, value in datatypes:
        tmpl = """def test_datatype_{0}(self):
    self.do_datatype_test(\"{1}\", {2})"""
        exec tmpl.format(datatype.replace(" ", "_"), datatype, value)

    int_modes = [
        ["DI", "int64"],
        ["__DI__", "int64"],
        ["SI", "int32"],
        ["__SI__", "int32"],
        ["HI", "int16"],
        ["__HI__", "int16"],
        ["QI", "int8"],
        ["__QI__", "int8"],
        ["pointer", "int32"],
        ["__pointer__", "int32"]]
    for mode, expected in int_modes:
        exec """def test_signed_int_mode_{0}(self):
    self.do_mode_test(\"int\", \"{0}\", \"{1}\")""".format(mode, expected)
    for mode, expected in int_modes:
        exec """def test_unsigned_int_mode_{0}(self):
    self.do_mode_test(\"unsigned\", \"{0}\", \"{1}\")""".format(mode, "u" + expected)

    float_modes = [
        ["SF", "float"],
        ["__SF__", "float"],
        ["DF", "longdouble"],
        ["__DF__", "longdouble"]]
    for mode, expected in float_modes:
        exec """def test_float_mode_{0}(self):
    self.do_mode_test(\"float\", \"{0}\", \"{1}\")""".format(mode, expected)

# vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
