# -*- coding: utf-8 -*-

from os.path import join
from testcase import TestCase
import os

class TestFFI(TestCase):

    def get_lib_path(self):
        return join(".", "test_lib" + ".so" if os.name == "posix" else ".dll")

    def test_load_lib0(self):
        path = self.get_lib_path()
        self._test("load_lib(\"%(path)s\")" % locals())

    def test_load_func0(self):
        path = self.get_lib_path()
        self._test("""
lib = load_lib(\"%(path)s\")
f = lib.load_func(\"foo\")
f()
""" % locals(), "42")

    # Tests for uint8
    def test_Struct10(self):
        def test_stderr(stderr):
            assert 0 < stderr.find("ValueError: Value must be greater or equal 0, not -1")
        self._test("""
Foo = StructClass.new(\"Foo\", [[\'uint8, \'bar]])
foo = Foo.new()
foo.bar = -1
""", stderr=test_stderr)

    def test_Struct20(self):
        self._test("""
Foo = StructClass.new(\"Foo\", [[\'uint8, \'bar]])
foo = Foo.new()
foo.bar = 0
print(foo.bar)
""", "0")

    def test_Struct30(self):
        self._test("""
Foo = StructClass.new(\"Foo\", [[\'uint8, \'bar]])
foo = Foo.new()
foo.bar = 255
print(foo.bar)
""", "255")

    def test_Struct40(self):
        def test_stderr(stderr):
            assert 0 < stderr.find("ValueError: Value must be less or equal 255, not 256")
        self._test("""
Foo = StructClass.new(\"Foo\", [[\'uint8, \'bar]])
foo = Foo.new()
foo.bar = 256
""", stderr=test_stderr)

    def test_Struct50(self):
        def test_stderr(stderr):
            assert 0 < stderr.find("TypeError: Value must be Fixnum, not String")
        self._test("""
Foo = StructClass.new(\"Foo\", [[\'uint8, \'bar]])
foo = Foo.new()
foo.bar = \"baz\"
""", stderr=test_stderr)

    # Tests for int8
    def test_Struct60(self):
        def test_stderr(stderr):
            assert 0 < stderr.find("ValueError: Value must be greater or equal -128, not -129")
        self._test("""
Foo = StructClass.new(\"Foo\", [[\'int8, \'bar]])
foo = Foo.new()
foo.bar = -129
""", stderr=test_stderr)

    def test_Struct70(self):
        self._test("""
Foo = StructClass.new(\"Foo\", [[\'int8, \'bar]])
foo = Foo.new()
foo.bar = -128
print(foo.bar)
""", "-128")

    def test_Struct80(self):
        self._test("""
Foo = StructClass.new(\"Foo\", [[\'int8, \'bar]])
foo = Foo.new()
foo.bar = 127
print(foo.bar)
""", "127")

    def test_Struct90(self):
        def test_stderr(stderr):
            assert 0 < stderr.find("ValueError: Value must be less or equal 127, not 128")
        self._test("""
Foo = StructClass.new(\"Foo\", [[\'int8, \'bar]])
foo = Foo.new()
foo.bar = 128
""", stderr=test_stderr)

    def test_Struct100(self):
        def test_stderr(stderr):
            assert 0 < stderr.find("TypeError: Value must be Fixnum, not String")
        self._test("""
Foo = StructClass.new(\"Foo\", [[\'int8, \'bar]])
foo = Foo.new()
foo.bar = \"baz\"
""", stderr=test_stderr)

    # Tests for uint16
    def test_Struct110(self):
        def test_stderr(stderr):
            assert 0 < stderr.find("ValueError: Value must be greater or equal 0, not -1")
        self._test("""
Foo = StructClass.new(\"Foo\", [[\'uint16, \'bar]])
foo = Foo.new()
foo.bar = -1
""", stderr=test_stderr)

    def test_Struct120(self):
        self._test("""
Foo = StructClass.new(\"Foo\", [[\'uint16, \'bar]])
foo = Foo.new()
foo.bar = 0
print(foo.bar)
""", "0")

    def test_Struct130(self):
        self._test("""
Foo = StructClass.new(\"Foo\", [[\'uint16, \'bar]])
foo = Foo.new()
foo.bar = 65535
print(foo.bar)
""", "65535")

    def test_Struct140(self):
        def test_stderr(stderr):
            assert 0 < stderr.find("ValueError: Value must be less or equal 65535, not 65536")
        self._test("""
Foo = StructClass.new(\"Foo\", [[\'uint16, \'bar]])
foo = Foo.new()
foo.bar = 65536
""", stderr=test_stderr)

    def test_Struct150(self):
        def test_stderr(stderr):
            assert 0 < stderr.find("TypeError: Value must be Fixnum, not String")
        self._test("""
Foo = StructClass.new(\"Foo\", [[\'uint16, \'bar]])
foo = Foo.new()
foo.bar = \"baz\"
""", stderr=test_stderr)

    # Tests for int16
    def test_Struct160(self):
        def test_stderr(stderr):
            assert 0 < stderr.find("ValueError: Value must be greater or equal -32768, not -32769")
        self._test("""
Foo = StructClass.new(\"Foo\", [[\'int16, \'bar]])
foo = Foo.new()
foo.bar = -32769
""", stderr=test_stderr)

    def test_Struct170(self):
        self._test("""
Foo = StructClass.new(\"Foo\", [[\'int16, \'bar]])
foo = Foo.new()
foo.bar = -32768
print(foo.bar)
""", "-32768")

    def test_Struct180(self):
        self._test("""
Foo = StructClass.new(\"Foo\", [[\'int16, \'bar]])
foo = Foo.new()
foo.bar = 32767
print(foo.bar)
""", "32767")

    def test_Struct190(self):
        def test_stderr(stderr):
            assert 0 < stderr.find("ValueError: Value must be less or equal 32767, not 32768")
        self._test("""
Foo = StructClass.new(\"Foo\", [[\'int16, \'bar]])
foo = Foo.new()
foo.bar = 32768
""", stderr=test_stderr)

    def test_Struct200(self):
        def test_stderr(stderr):
            assert 0 < stderr.find("TypeError: Value must be Fixnum, not String")
        self._test("""
Foo = StructClass.new(\"Foo\", [[\'int16, \'bar]])
foo = Foo.new()
foo.bar = \"baz\"
""", stderr=test_stderr)

    # Tests for uint32
    def test_Struct210(self):
        def test_stderr(stderr):
            assert 0 < stderr.find("ValueError: Value must be greater or equal 0, not -1")
        self._test("""
Foo = StructClass.new(\"Foo\", [[\'uint32, \'bar]])
foo = Foo.new()
foo.bar = -1
""", stderr=test_stderr)

    def test_Struct220(self):
        self._test("""
Foo = StructClass.new(\"Foo\", [[\'uint32, \'bar]])
foo = Foo.new()
foo.bar = 0
print(foo.bar)
""", "0")

    def test_Struct230(self):
        self._test("""
Foo = StructClass.new(\"Foo\", [[\'uint32, \'bar]])
foo = Foo.new()
foo.bar = 4294967295
print(foo.bar)
""", "4294967295")

    def test_Struct240(self):
        def test_stderr(stderr):
            assert 0 < stderr.find("ValueError: Value must be less or equal 4294967295, not 4294967296")
        self._test("""
Foo = StructClass.new(\"Foo\", [[\'uint32, \'bar]])
foo = Foo.new()
foo.bar = 4294967296
""", stderr=test_stderr)

    def test_Struct250(self):
        def test_stderr(stderr):
            assert 0 < stderr.find("TypeError: Value must be Fixnum or Bignum, not String")
        self._test("""
Foo = StructClass.new(\"Foo\", [[\'uint32, \'bar]])
foo = Foo.new()
foo.bar = \"baz\"
""", stderr=test_stderr)

    # Tests for uint32
    def test_Struct260(self):
        def test_stderr(stderr):
            assert 0 < stderr.find("ValueError: Value must be greater or equal 0, not -1")
        self._test("""
Foo = StructClass.new(\"Foo\", [[\'uint32, \'bar]])
foo = Foo.new()
foo.bar = -1
""", stderr=test_stderr)

    def test_Struct270(self):
        self._test("""
Foo = StructClass.new(\"Foo\", [[\'uint32, \'bar]])
foo = Foo.new()
foo.bar = 0
print(foo.bar)
""", "0")

    def test_Struct280(self):
        self._test("""
Foo = StructClass.new(\"Foo\", [[\'uint32, \'bar]])
foo = Foo.new()
foo.bar = 4294967295
print(foo.bar)
""", "4294967295")

    def test_Struct290(self):
        def test_stderr(stderr):
            assert 0 < stderr.find("ValueError: Value must be less or equal 4294967295, not 4294967296")
        self._test("""
Foo = StructClass.new(\"Foo\", [[\'uint32, \'bar]])
foo = Foo.new()
foo.bar = 4294967296
""", stderr=test_stderr)

    def test_Struct300(self):
        def test_stderr(stderr):
            assert 0 < stderr.find("TypeError: Value must be Fixnum or Bignum, not String")
        self._test("""
Foo = StructClass.new(\"Foo\", [[\'uint32, \'bar]])
foo = Foo.new()
foo.bar = \"baz\"
""", stderr=test_stderr)

    # Tests for int32
    def test_Struct310(self):
        def test_stderr(stderr):
            assert 0 < stderr.find("ValueError: Value must be greater or equal -2147483648, not -2147483649")
        self._test("""
Foo = StructClass.new(\"Foo\", [[\'int32, \'bar]])
foo = Foo.new()
foo.bar = -2147483649
""", stderr=test_stderr)

    def test_Struct320(self):
        self._test("""
Foo = StructClass.new(\"Foo\", [[\'int32, \'bar]])
foo = Foo.new()
foo.bar = -2147483648
print(foo.bar)
""", "-2147483648")

    def test_Struct330(self):
        self._test("""
Foo = StructClass.new(\"Foo\", [[\'int32, \'bar]])
foo = Foo.new()
foo.bar = 2147483647
print(foo.bar)
""", "2147483647")

    def test_Struct340(self):
        def test_stderr(stderr):
            assert 0 < stderr.find("ValueError: Value must be less or equal 2147483647, not 2147483648")
        self._test("""
Foo = StructClass.new(\"Foo\", [[\'int32, \'bar]])
foo = Foo.new()
foo.bar = 2147483648
""", stderr=test_stderr)

    def test_Struct350(self):
        def test_stderr(stderr):
            assert 0 < stderr.find("TypeError: Value must be Fixnum or Bignum, not String")
        self._test("""
Foo = StructClass.new(\"Foo\", [[\'int32, \'bar]])
foo = Foo.new()
foo.bar = \"baz\"
""", stderr=test_stderr)

    # Tests for uint64
    def test_Struct360(self):
        def test_stderr(stderr):
            assert 0 < stderr.find("ValueError: Value must be greater or equal 0, not -1")
        self._test("""
Foo = StructClass.new(\"Foo\", [[\'uint64, \'bar]])
foo = Foo.new()
foo.bar = -1
""", stderr=test_stderr)

    def test_Struct370(self):
        self._test("""
Foo = StructClass.new(\"Foo\", [[\'uint64, \'bar]])
foo = Foo.new()
foo.bar = 0
print(foo.bar)
""", "0")

    def test_Struct380(self):
        self._test("""
Foo = StructClass.new(\"Foo\", [[\'uint64, \'bar]])
foo = Foo.new()
foo.bar = 18446744073709551615
print(foo.bar)
""", "18446744073709551615")

    def test_Struct390(self):
        def test_stderr(stderr):
            assert 0 < stderr.find("ValueError: Value must be less or equal 18446744073709551615, not 18446744073709551616")
        self._test("""
Foo = StructClass.new(\"Foo\", [[\'uint64, \'bar]])
foo = Foo.new()
foo.bar = 18446744073709551616
""", stderr=test_stderr)

    def test_Struct400(self):
        def test_stderr(stderr):
            assert 0 < stderr.find("TypeError: Value must be Fixnum or Bignum, not String")
        self._test("""
Foo = StructClass.new(\"Foo\", [[\'uint64, \'bar]])
foo = Foo.new()
foo.bar = \"baz\"
""", stderr=test_stderr)

    # Tests for int64
    def test_Struct410(self):
        def test_stderr(stderr):
            assert 0 < stderr.find("ValueError: Value must be greater or equal -9223372036854775808, not -9223372036854775809")
        self._test("""
Foo = StructClass.new(\"Foo\", [[\'int64, \'bar]])
foo = Foo.new()
foo.bar = -9223372036854775809
""", stderr=test_stderr)

    def test_Struct420(self):
        self._test("""
Foo = StructClass.new(\"Foo\", [[\'int64, \'bar]])
foo = Foo.new()
foo.bar = -9223372036854775808
print(foo.bar)
""", "-9223372036854775808")

    def test_Struct430(self):
        self._test("""
Foo = StructClass.new(\"Foo\", [[\'int64, \'bar]])
foo = Foo.new()
foo.bar = 9223372036854775807
print(foo.bar)
""", "9223372036854775807")

    def test_Struct440(self):
        def test_stderr(stderr):
            assert 0 < stderr.find("ValueError: Value must be less or equal 9223372036854775807, not 9223372036854775808")
        self._test("""
Foo = StructClass.new(\"Foo\", [[\'int64, \'bar]])
foo = Foo.new()
foo.bar = 9223372036854775808
""", stderr=test_stderr)

    def test_Struct450(self):
        def test_stderr(stderr):
            assert 0 < stderr.find("TypeError: Value must be Fixnum or Bignum, not String")
        self._test("""
Foo = StructClass.new(\"Foo\", [[\'int64, \'bar]])
foo = Foo.new()
foo.bar = \"baz\"
""", stderr=test_stderr)

    # Tests for float
    def test_Struct460(self):
        self._test("""
Foo = StructClass.new(\"Foo\", [[\'float, \'bar]])
foo = Foo.new()
foo.bar = 3.14
print(foo.bar)
""", "3.1400001049")

    def test_Struct470(self):
        def test_stderr(stderr):
            assert 0 < stderr.find("TypeError: Value must be Float, not String")
        self._test("""
Foo = StructClass.new(\"Foo\", [[\'float, \'bar]])
foo = Foo.new()
foo.bar = \"baz\"
""", stderr=test_stderr)

    # Tests for double
    def test_Struct480(self):
        self._test("""
Foo = StructClass.new(\"Foo\", [[\'double, \'bar]])
foo = Foo.new()
foo.bar = 3.14
print(foo.bar)
""", "3.14")

    def test_Struct490(self):
        def test_stderr(stderr):
            assert 0 < stderr.find("TypeError: Value must be Float, not String")
        self._test("""
Foo = StructClass.new(\"Foo\", [[\'double, \'bar]])
foo = Foo.new()
foo.bar = \"baz\"
""", stderr=test_stderr)

    # Tests for uchar
    def test_Struct500(self):
        def test_stderr(stderr):
            assert 0 < stderr.find("ValueError: Value must be greater or equal 0, not -1")
        self._test("""
Foo = StructClass.new(\"Foo\", [[\'uchar, \'bar]])
foo = Foo.new()
foo.bar = -1
""", stderr=test_stderr)

    def test_Struct510(self):
        self._test("""
Foo = StructClass.new(\"Foo\", [[\'uchar, \'bar]])
foo = Foo.new()
foo.bar = 0
print(foo.bar)
""", "0")

    def test_Struct520(self):
        self._test("""
Foo = StructClass.new(\"Foo\", [[\'uchar, \'bar]])
foo = Foo.new()
foo.bar = 255
print(foo.bar)
""", "255")

    def test_Struct530(self):
        def test_stderr(stderr):
            assert 0 < stderr.find("ValueError: Value must be less or equal 255, not 256")
        self._test("""
Foo = StructClass.new(\"Foo\", [[\'uchar, \'bar]])
foo = Foo.new()
foo.bar = 256
""", stderr=test_stderr)

    def test_Struct540(self):
        def test_stderr(stderr):
            assert 0 < stderr.find("TypeError: Value must be Fixnum, not String")
        self._test("""
Foo = StructClass.new(\"Foo\", [[\'uchar, \'bar]])
foo = Foo.new()
foo.bar = \"baz\"
""", stderr=test_stderr)

    # Tests for char
    def test_Struct550(self):
        def test_stderr(stderr):
            assert 0 < stderr.find("ValueError: Value must be greater or equal -128, not -129")
        self._test("""
Foo = StructClass.new(\"Foo\", [[\'char, \'bar]])
foo = Foo.new()
foo.bar = -129
""", stderr=test_stderr)

    def test_Struct560(self):
        self._test("""
Foo = StructClass.new(\"Foo\", [[\'char, \'bar]])
foo = Foo.new()
foo.bar = -128
print(foo.bar)
""", "-128")

    def test_Struct570(self):
        self._test("""
Foo = StructClass.new(\"Foo\", [[\'char, \'bar]])
foo = Foo.new()
foo.bar = 127
print(foo.bar)
""", "127")

    def test_Struct580(self):
        def test_stderr(stderr):
            assert 0 < stderr.find("ValueError: Value must be less or equal 127, not 128")
        self._test("""
Foo = StructClass.new(\"Foo\", [[\'char, \'bar]])
foo = Foo.new()
foo.bar = 128
""", stderr=test_stderr)

    def test_Struct590(self):
        def test_stderr(stderr):
            assert 0 < stderr.find("TypeError: Value must be Fixnum, not String")
        self._test("""
Foo = StructClass.new(\"Foo\", [[\'char, \'bar]])
foo = Foo.new()
foo.bar = \"baz\"
""", stderr=test_stderr)

    # Tests for longdouble
    def test_Struct600(self):
        self._test("""
Foo = StructClass.new(\"Foo\", [[\'longdouble, \'bar]])
foo = Foo.new()
foo.bar = 3.14
print(foo.bar)
""", "3.14")

    def test_Struct610(self):
        def test_stderr(stderr):
            assert 0 < stderr.find("TypeError: Value must be Float, not String")
        self._test("""
Foo = StructClass.new(\"Foo\", [[\'longdouble, \'bar]])
foo = Foo.new()
foo.bar = \"baz\"
""", stderr=test_stderr)

    # Tests for uint8
    def test_argument10(self):
        def test_stderr(stderr):
            assert 0 < stderr.find("ValueError: Value must be greater or equal 0, not -1")
        path = self.get_lib_path()
        self._test("""
lib = load_lib(\"%(path)s\")
f = lib.load_func(\"print_uint8\", [\'uint8])
f(-1)
""" % locals(), stderr=test_stderr)

    def test_argument20(self):
        path = self.get_lib_path()
        self._test("""
lib = load_lib(\"%(path)s\")
f = lib.load_func(\"print_uint8\", [\'uint8])
f(0)
""" % locals(), "0")

    def test_argument30(self):
        path = self.get_lib_path()
        self._test("""
lib = load_lib(\"%(path)s\")
f = lib.load_func(\"print_uint8\", [\'uint8])
f(255)
""" % locals(), "255")

    def test_argument40(self):
        def test_stderr(stderr):
            assert 0 < stderr.find("ValueError: Value must be less or equal 255, not 256")
        path = self.get_lib_path()
        self._test("""
lib = load_lib(\"%(path)s\")
f = lib.load_func(\"print_uint8\", [\'uint8])
f(256)
""" % locals(), stderr=test_stderr)

    def test_argument50(self):
        def test_stderr(stderr):
            assert 0 < stderr.find("TypeError: Value must be Fixnum, not String")
        path = self.get_lib_path()
        self._test("""
lib = load_lib(\"%(path)s\")
f = lib.load_func(\"print_uint8\", [\'uint8])
f(\"baz\")
""" % locals(), stderr=test_stderr)

    # Tests for int8
    def test_argument60(self):
        def test_stderr(stderr):
            assert 0 < stderr.find("ValueError: Value must be greater or equal -128, not -129")
        path = self.get_lib_path()
        self._test("""
lib = load_lib(\"%(path)s\")
f = lib.load_func(\"print_int8\", [\'int8])
f(-129)
""" % locals(), stderr=test_stderr)

    def test_argument70(self):
        path = self.get_lib_path()
        self._test("""
lib = load_lib(\"%(path)s\")
f = lib.load_func(\"print_int8\", [\'int8])
f(-128)
""" % locals(), "-128")

    def test_argument80(self):
        path = self.get_lib_path()
        self._test("""
lib = load_lib(\"%(path)s\")
f = lib.load_func(\"print_int8\", [\'int8])
f(127)
""" % locals(), "127")

    def test_argument90(self):
        def test_stderr(stderr):
            assert 0 < stderr.find("ValueError: Value must be less or equal 127, not 128")
        path = self.get_lib_path()
        self._test("""
lib = load_lib(\"%(path)s\")
f = lib.load_func(\"print_int8\", [\'int8])
f(128)
""" % locals(), stderr=test_stderr)

    def test_argument100(self):
        def test_stderr(stderr):
            assert 0 < stderr.find("TypeError: Value must be Fixnum, not String")
        path = self.get_lib_path()
        self._test("""
lib = load_lib(\"%(path)s\")
f = lib.load_func(\"print_int8\", [\'int8])
f(\"baz\")
""" % locals(), stderr=test_stderr)

    # Tests for uint16
    def test_argument110(self):
        def test_stderr(stderr):
            assert 0 < stderr.find("ValueError: Value must be greater or equal 0, not -1")
        path = self.get_lib_path()
        self._test("""
lib = load_lib(\"%(path)s\")
f = lib.load_func(\"print_uint16\", [\'uint16])
f(-1)
""" % locals(), stderr=test_stderr)

    def test_argument120(self):
        path = self.get_lib_path()
        self._test("""
lib = load_lib(\"%(path)s\")
f = lib.load_func(\"print_uint16\", [\'uint16])
f(0)
""" % locals(), "0")

    def test_argument130(self):
        path = self.get_lib_path()
        self._test("""
lib = load_lib(\"%(path)s\")
f = lib.load_func(\"print_uint16\", [\'uint16])
f(65535)
""" % locals(), "65535")

    def test_argument140(self):
        def test_stderr(stderr):
            assert 0 < stderr.find("ValueError: Value must be less or equal 65535, not 65536")
        path = self.get_lib_path()
        self._test("""
lib = load_lib(\"%(path)s\")
f = lib.load_func(\"print_uint16\", [\'uint16])
f(65536)
""" % locals(), stderr=test_stderr)

    def test_argument150(self):
        def test_stderr(stderr):
            assert 0 < stderr.find("TypeError: Value must be Fixnum, not String")
        path = self.get_lib_path()
        self._test("""
lib = load_lib(\"%(path)s\")
f = lib.load_func(\"print_uint16\", [\'uint16])
f(\"baz\")
""" % locals(), stderr=test_stderr)

    # Tests for int16
    def test_argument160(self):
        def test_stderr(stderr):
            assert 0 < stderr.find("ValueError: Value must be greater or equal -32768, not -32769")
        path = self.get_lib_path()
        self._test("""
lib = load_lib(\"%(path)s\")
f = lib.load_func(\"print_int16\", [\'int16])
f(-32769)
""" % locals(), stderr=test_stderr)

    def test_argument170(self):
        path = self.get_lib_path()
        self._test("""
lib = load_lib(\"%(path)s\")
f = lib.load_func(\"print_int16\", [\'int16])
f(-32768)
""" % locals(), "-32768")

    def test_argument180(self):
        path = self.get_lib_path()
        self._test("""
lib = load_lib(\"%(path)s\")
f = lib.load_func(\"print_int16\", [\'int16])
f(32767)
""" % locals(), "32767")

    def test_argument190(self):
        def test_stderr(stderr):
            assert 0 < stderr.find("ValueError: Value must be less or equal 32767, not 32768")
        path = self.get_lib_path()
        self._test("""
lib = load_lib(\"%(path)s\")
f = lib.load_func(\"print_int16\", [\'int16])
f(32768)
""" % locals(), stderr=test_stderr)

    def test_argument200(self):
        def test_stderr(stderr):
            assert 0 < stderr.find("TypeError: Value must be Fixnum, not String")
        path = self.get_lib_path()
        self._test("""
lib = load_lib(\"%(path)s\")
f = lib.load_func(\"print_int16\", [\'int16])
f(\"baz\")
""" % locals(), stderr=test_stderr)

    # Tests for uint32
    def test_argument210(self):
        def test_stderr(stderr):
            assert 0 < stderr.find("ValueError: Value must be greater or equal 0, not -1")
        path = self.get_lib_path()
        self._test("""
lib = load_lib(\"%(path)s\")
f = lib.load_func(\"print_uint32\", [\'uint32])
f(-1)
""" % locals(), stderr=test_stderr)

    def test_argument220(self):
        path = self.get_lib_path()
        self._test("""
lib = load_lib(\"%(path)s\")
f = lib.load_func(\"print_uint32\", [\'uint32])
f(0)
""" % locals(), "0")

    def test_argument230(self):
        path = self.get_lib_path()
        self._test("""
lib = load_lib(\"%(path)s\")
f = lib.load_func(\"print_uint32\", [\'uint32])
f(4294967295)
""" % locals(), "4294967295")

    def test_argument240(self):
        def test_stderr(stderr):
            assert 0 < stderr.find("ValueError: Value must be less or equal 4294967295, not 4294967296")
        path = self.get_lib_path()
        self._test("""
lib = load_lib(\"%(path)s\")
f = lib.load_func(\"print_uint32\", [\'uint32])
f(4294967296)
""" % locals(), stderr=test_stderr)

    def test_argument250(self):
        def test_stderr(stderr):
            assert 0 < stderr.find("TypeError: Value must be Fixnum or Bignum, not String")
        path = self.get_lib_path()
        self._test("""
lib = load_lib(\"%(path)s\")
f = lib.load_func(\"print_uint32\", [\'uint32])
f(\"baz\")
""" % locals(), stderr=test_stderr)

    # Tests for int32
    def test_argument260(self):
        def test_stderr(stderr):
            assert 0 < stderr.find("ValueError: Value must be greater or equal -2147483648, not -2147483649")
        path = self.get_lib_path()
        self._test("""
lib = load_lib(\"%(path)s\")
f = lib.load_func(\"print_int32\", [\'int32])
f(-2147483649)
""" % locals(), stderr=test_stderr)

    def test_argument270(self):
        path = self.get_lib_path()
        self._test("""
lib = load_lib(\"%(path)s\")
f = lib.load_func(\"print_int32\", [\'int32])
f(-2147483648)
""" % locals(), "-2147483648")

    def test_argument280(self):
        path = self.get_lib_path()
        self._test("""
lib = load_lib(\"%(path)s\")
f = lib.load_func(\"print_int32\", [\'int32])
f(2147483647)
""" % locals(), "2147483647")

    def test_argument290(self):
        def test_stderr(stderr):
            assert 0 < stderr.find("ValueError: Value must be less or equal 2147483647, not 2147483648")
        path = self.get_lib_path()
        self._test("""
lib = load_lib(\"%(path)s\")
f = lib.load_func(\"print_int32\", [\'int32])
f(2147483648)
""" % locals(), stderr=test_stderr)

    def test_argument300(self):
        def test_stderr(stderr):
            assert 0 < stderr.find("TypeError: Value must be Fixnum or Bignum, not String")
        path = self.get_lib_path()
        self._test("""
lib = load_lib(\"%(path)s\")
f = lib.load_func(\"print_int32\", [\'int32])
f(\"baz\")
""" % locals(), stderr=test_stderr)

    # Tests for int32
    def test_argument310(self):
        def test_stderr(stderr):
            assert 0 < stderr.find("ValueError: Value must be greater or equal -2147483648, not -2147483649")
        path = self.get_lib_path()
        self._test("""
lib = load_lib(\"%(path)s\")
f = lib.load_func(\"print_int32\", [\'int32])
f(-2147483649)
""" % locals(), stderr=test_stderr)

    def test_argument320(self):
        path = self.get_lib_path()
        self._test("""
lib = load_lib(\"%(path)s\")
f = lib.load_func(\"print_int32\", [\'int32])
f(-2147483648)
""" % locals(), "-2147483648")

    def test_argument330(self):
        path = self.get_lib_path()
        self._test("""
lib = load_lib(\"%(path)s\")
f = lib.load_func(\"print_int32\", [\'int32])
f(2147483647)
""" % locals(), "2147483647")

    def test_argument340(self):
        def test_stderr(stderr):
            assert 0 < stderr.find("ValueError: Value must be less or equal 2147483647, not 2147483648")
        path = self.get_lib_path()
        self._test("""
lib = load_lib(\"%(path)s\")
f = lib.load_func(\"print_int32\", [\'int32])
f(2147483648)
""" % locals(), stderr=test_stderr)

    def test_argument350(self):
        def test_stderr(stderr):
            assert 0 < stderr.find("TypeError: Value must be Fixnum or Bignum, not String")
        path = self.get_lib_path()
        self._test("""
lib = load_lib(\"%(path)s\")
f = lib.load_func(\"print_int32\", [\'int32])
f(\"baz\")
""" % locals(), stderr=test_stderr)

    # Tests for uint64
    def test_argument360(self):
        def test_stderr(stderr):
            assert 0 < stderr.find("ValueError: Value must be greater or equal 0, not -1")
        path = self.get_lib_path()
        self._test("""
lib = load_lib(\"%(path)s\")
f = lib.load_func(\"print_uint64\", [\'uint64])
f(-1)
""" % locals(), stderr=test_stderr)

    def test_argument370(self):
        path = self.get_lib_path()
        self._test("""
lib = load_lib(\"%(path)s\")
f = lib.load_func(\"print_uint64\", [\'uint64])
f(0)
""" % locals(), "0")

    def test_argument380(self):
        path = self.get_lib_path()
        self._test("""
lib = load_lib(\"%(path)s\")
f = lib.load_func(\"print_uint64\", [\'uint64])
f(18446744073709551615)
""" % locals(), "18446744073709551615")

    def test_argument390(self):
        def test_stderr(stderr):
            assert 0 < stderr.find("ValueError: Value must be less or equal 18446744073709551615, not 18446744073709551616")
        path = self.get_lib_path()
        self._test("""
lib = load_lib(\"%(path)s\")
f = lib.load_func(\"print_uint64\", [\'uint64])
f(18446744073709551616)
""" % locals(), stderr=test_stderr)

    def test_argument400(self):
        def test_stderr(stderr):
            assert 0 < stderr.find("TypeError: Value must be Fixnum or Bignum, not String")
        path = self.get_lib_path()
        self._test("""
lib = load_lib(\"%(path)s\")
f = lib.load_func(\"print_uint64\", [\'uint64])
f(\"baz\")
""" % locals(), stderr=test_stderr)

    # Tests for int64
    def test_argument410(self):
        def test_stderr(stderr):
            assert 0 < stderr.find("ValueError: Value must be greater or equal -9223372036854775808, not -9223372036854775809")
        path = self.get_lib_path()
        self._test("""
lib = load_lib(\"%(path)s\")
f = lib.load_func(\"print_int64\", [\'int64])
f(-9223372036854775809)
""" % locals(), stderr=test_stderr)

    def test_argument420(self):
        path = self.get_lib_path()
        self._test("""
lib = load_lib(\"%(path)s\")
f = lib.load_func(\"print_int64\", [\'int64])
f(-9223372036854775808)
""" % locals(), "-9223372036854775808")

    def test_argument430(self):
        path = self.get_lib_path()
        self._test("""
lib = load_lib(\"%(path)s\")
f = lib.load_func(\"print_int64\", [\'int64])
f(9223372036854775807)
""" % locals(), "9223372036854775807")

    def test_argument440(self):
        def test_stderr(stderr):
            assert 0 < stderr.find("ValueError: Value must be less or equal 9223372036854775807, not 9223372036854775808")
        path = self.get_lib_path()
        self._test("""
lib = load_lib(\"%(path)s\")
f = lib.load_func(\"print_int64\", [\'int64])
f(9223372036854775808)
""" % locals(), stderr=test_stderr)

    def test_argument450(self):
        def test_stderr(stderr):
            assert 0 < stderr.find("TypeError: Value must be Fixnum or Bignum, not String")
        path = self.get_lib_path()
        self._test("""
lib = load_lib(\"%(path)s\")
f = lib.load_func(\"print_int64\", [\'int64])
f(\"baz\")
""" % locals(), stderr=test_stderr)

    # Tests for float
    def test_argument460(self):
        path = self.get_lib_path()
        self._test("""
lib = load_lib(\"%(path)s\")
f = lib.load_func(\"print_float\", [\'float])
f(3.14)
""" % locals(), "3.14")

    def test_argument470(self):
        def test_stderr(stderr):
            assert 0 < stderr.find("TypeError: Value must be Float, not String")
        path = self.get_lib_path()
        self._test("""
lib = load_lib(\"%(path)s\")
f = lib.load_func(\"print_float\", [\'float])
f(\"baz\")
""" % locals(), stderr=test_stderr)

    # Tests for double
    def test_argument480(self):
        path = self.get_lib_path()
        self._test("""
lib = load_lib(\"%(path)s\")
f = lib.load_func(\"print_double\", [\'double])
f(3.14)
""" % locals(), "3.14")

    def test_argument490(self):
        def test_stderr(stderr):
            assert 0 < stderr.find("TypeError: Value must be Float, not String")
        path = self.get_lib_path()
        self._test("""
lib = load_lib(\"%(path)s\")
f = lib.load_func(\"print_double\", [\'double])
f(\"baz\")
""" % locals(), stderr=test_stderr)

    # Tests for uchar
    def test_argument500(self):
        def test_stderr(stderr):
            assert 0 < stderr.find("ValueError: Value must be greater or equal 0, not -1")
        path = self.get_lib_path()
        self._test("""
lib = load_lib(\"%(path)s\")
f = lib.load_func(\"print_uchar\", [\'uchar])
f(-1)
""" % locals(), stderr=test_stderr)

    def test_argument510(self):
        path = self.get_lib_path()
        self._test("""
lib = load_lib(\"%(path)s\")
f = lib.load_func(\"print_uchar\", [\'uchar])
f(0)
""" % locals(), "0")

    def test_argument520(self):
        path = self.get_lib_path()
        self._test("""
lib = load_lib(\"%(path)s\")
f = lib.load_func(\"print_uchar\", [\'uchar])
f(255)
""" % locals(), "255")

    def test_argument530(self):
        def test_stderr(stderr):
            assert 0 < stderr.find("ValueError: Value must be less or equal 255, not 256")
        path = self.get_lib_path()
        self._test("""
lib = load_lib(\"%(path)s\")
f = lib.load_func(\"print_uchar\", [\'uchar])
f(256)
""" % locals(), stderr=test_stderr)

    def test_argument540(self):
        def test_stderr(stderr):
            assert 0 < stderr.find("TypeError: Value must be Fixnum, not String")
        path = self.get_lib_path()
        self._test("""
lib = load_lib(\"%(path)s\")
f = lib.load_func(\"print_uchar\", [\'uchar])
f(\"baz\")
""" % locals(), stderr=test_stderr)

    # Tests for char
    def test_argument550(self):
        def test_stderr(stderr):
            assert 0 < stderr.find("ValueError: Value must be greater or equal -128, not -129")
        path = self.get_lib_path()
        self._test("""
lib = load_lib(\"%(path)s\")
f = lib.load_func(\"print_char\", [\'char])
f(-129)
""" % locals(), stderr=test_stderr)

    def test_argument560(self):
        path = self.get_lib_path()
        self._test("""
lib = load_lib(\"%(path)s\")
f = lib.load_func(\"print_char\", [\'char])
f(-128)
""" % locals(), "-128")

    def test_argument570(self):
        path = self.get_lib_path()
        self._test("""
lib = load_lib(\"%(path)s\")
f = lib.load_func(\"print_char\", [\'char])
f(127)
""" % locals(), "127")

    def test_argument580(self):
        def test_stderr(stderr):
            assert 0 < stderr.find("ValueError: Value must be less or equal 127, not 128")
        path = self.get_lib_path()
        self._test("""
lib = load_lib(\"%(path)s\")
f = lib.load_func(\"print_char\", [\'char])
f(128)
""" % locals(), stderr=test_stderr)

    def test_argument590(self):
        def test_stderr(stderr):
            assert 0 < stderr.find("TypeError: Value must be Fixnum, not String")
        path = self.get_lib_path()
        self._test("""
lib = load_lib(\"%(path)s\")
f = lib.load_func(\"print_char\", [\'char])
f(\"baz\")
""" % locals(), stderr=test_stderr)

    # Tests for longdouble
    def test_argument600(self):
        path = self.get_lib_path()
        self._test("""
lib = load_lib(\"%(path)s\")
f = lib.load_func(\"print_longdouble\", [\'longdouble])
f(3.14)
""" % locals(), "3.14")

    def test_argument610(self):
        def test_stderr(stderr):
            assert 0 < stderr.find("TypeError: Value must be Float, not String")
        path = self.get_lib_path()
        self._test("""
lib = load_lib(\"%(path)s\")
f = lib.load_func(\"print_longdouble\", [\'longdouble])
f(\"baz\")
""" % locals(), stderr=test_stderr)

    def test_arguments0(self):
        path = self.get_lib_path()
        self._test("""
lib = load_lib(\"%(path)s\")
f = lib.load_func(\"print_two_int\", [\'int, \'int])
f(42, 26)
""" % locals(), "4226")

    def test_return0(self):
        path = self.get_lib_path()
        self._test("""
lib = load_lib(\"%(path)s\")
f = lib.load_func(\"return_uint8\", [], \'uint8)
print(f())
""" % locals(), "42")

    def test_return10(self):
        path = self.get_lib_path()
        self._test("""
lib = load_lib(\"%(path)s\")
f = lib.load_func(\"return_int8\", [], \'int8)
print(f())
""" % locals(), "42")

    def test_return20(self):
        path = self.get_lib_path()
        self._test("""
lib = load_lib(\"%(path)s\")
f = lib.load_func(\"return_uint16\", [], \'uint16)
print(f())
""" % locals(), "42")

    def test_return30(self):
        path = self.get_lib_path()
        self._test("""
lib = load_lib(\"%(path)s\")
f = lib.load_func(\"return_int16\", [], \'int16)
print(f())
""" % locals(), "42")

    def test_return40(self):
        path = self.get_lib_path()
        self._test("""
lib = load_lib(\"%(path)s\")
f = lib.load_func(\"return_uint32_0\", [], \'uint32)
print(f())
""" % locals(), "1073741823")

    def test_return50(self):
        path = self.get_lib_path()
        self._test("""
lib = load_lib(\"%(path)s\")
f = lib.load_func(\"return_uint32_1\", [], \'uint32)
print(f())
""" % locals(), "1073741824")

    def test_return60(self):
        path = self.get_lib_path()
        self._test("""
lib = load_lib(\"%(path)s\")
f = lib.load_func(\"return_int32_0\", [], \'int32)
print(f())
""" % locals(), "-1073741825")

    def test_return70(self):
        path = self.get_lib_path()
        self._test("""
lib = load_lib(\"%(path)s\")
f = lib.load_func(\"return_int32_1\", [], \'int32)
print(f())
""" % locals(), "-1073741824")

    def test_return80(self):
        path = self.get_lib_path()
        self._test("""
lib = load_lib(\"%(path)s\")
f = lib.load_func(\"return_int32_2\", [], \'int32)
print(f())
""" % locals(), "1073741823")

    def test_return90(self):
        path = self.get_lib_path()
        self._test("""
lib = load_lib(\"%(path)s\")
f = lib.load_func(\"return_int32_3\", [], \'int32)
print(f())
""" % locals(), "1073741824")

    def test_return100(self):
        path = self.get_lib_path()
        self._test("""
lib = load_lib(\"%(path)s\")
f = lib.load_func(\"return_uint64_0\", [], \'uint64)
print(f())
""" % locals(), "1073741823")

    def test_return110(self):
        path = self.get_lib_path()
        self._test("""
lib = load_lib(\"%(path)s\")
f = lib.load_func(\"return_uint64_1\", [], \'uint64)
print(f())
""" % locals(), "1073741824")

    def test_return120(self):
        path = self.get_lib_path()
        self._test("""
lib = load_lib(\"%(path)s\")
f = lib.load_func(\"return_int64_0\", [], \'int64)
print(f())
""" % locals(), "-4611686018427387905")

    def test_return130(self):
        path = self.get_lib_path()
        self._test("""
lib = load_lib(\"%(path)s\")
f = lib.load_func(\"return_int64_1\", [], \'int64)
print(f())
""" % locals(), "-4611686018427387904")

    def test_return140(self):
        path = self.get_lib_path()
        self._test("""
lib = load_lib(\"%(path)s\")
f = lib.load_func(\"return_int64_2\", [], \'int64)
print(f())
""" % locals(), "-1073741825")

    def test_return150(self):
        path = self.get_lib_path()
        self._test("""
lib = load_lib(\"%(path)s\")
f = lib.load_func(\"return_int64_3\", [], \'int64)
print(f())
""" % locals(), "-1073741824")

    def test_return160(self):
        path = self.get_lib_path()
        self._test("""
lib = load_lib(\"%(path)s\")
f = lib.load_func(\"return_int64_4\", [], \'int64)
print(f())
""" % locals(), "1073741823")

    def test_return170(self):
        path = self.get_lib_path()
        self._test("""
lib = load_lib(\"%(path)s\")
f = lib.load_func(\"return_int64_5\", [], \'int64)
print(f())
""" % locals(), "1073741824")

    def test_return180(self):
        path = self.get_lib_path()
        self._test("""
lib = load_lib(\"%(path)s\")
f = lib.load_func(\"return_int64_6\", [], \'int64)
print(f())
""" % locals(), "4611686018427387903")

    def test_return190(self):
        path = self.get_lib_path()
        self._test("""
lib = load_lib(\"%(path)s\")
f = lib.load_func(\"return_int64_7\", [], \'int64)
print(f())
""" % locals(), "4611686018427387904")

    def test_return200(self):
        path = self.get_lib_path()
        self._test("""
lib = load_lib(\"%(path)s\")
f = lib.load_func(\"return_float\", [], \'float)
print(f())
""" % locals(), "3.14")

    def test_return210(self):
        path = self.get_lib_path()
        self._test("""
lib = load_lib(\"%(path)s\")
f = lib.load_func(\"return_double\", [], \'double)
print(f())
""" % locals(), "3.14")

    def test_return220(self):
        path = self.get_lib_path()
        self._test("""
lib = load_lib(\"%(path)s\")
f = lib.load_func(\"return_uchar\", [], \'uchar)
print(f())
""" % locals(), "42")

    def test_return230(self):
        path = self.get_lib_path()
        self._test("""
lib = load_lib(\"%(path)s\")
f = lib.load_func(\"return_char\", [], \'char)
print(f())
""" % locals(), "42")

    def test_return240(self):
        path = self.get_lib_path()
        self._test("""
lib = load_lib(\"%(path)s\")
f = lib.load_func(\"return_ushort\", [], \'ushort)
print(f())
""" % locals(), "42")

    def test_return250(self):
        path = self.get_lib_path()
        self._test("""
lib = load_lib(\"%(path)s\")
f = lib.load_func(\"return_short\", [], \'short)
print(f())
""" % locals(), "42")

    def test_return260(self):
        path = self.get_lib_path()
        self._test("""
lib = load_lib(\"%(path)s\")
f = lib.load_func(\"return_uint_0\", [], \'uint)
print(f())
""" % locals(), "1073741823")

    def test_return270(self):
        path = self.get_lib_path()
        self._test("""
lib = load_lib(\"%(path)s\")
f = lib.load_func(\"return_uint_1\", [], \'uint)
print(f())
""" % locals(), "1073741824")

    def test_return280(self):
        path = self.get_lib_path()
        self._test("""
lib = load_lib(\"%(path)s\")
f = lib.load_func(\"return_int_0\", [], \'int)
print(f())
""" % locals(), "-1073741825")

    def test_return290(self):
        path = self.get_lib_path()
        self._test("""
lib = load_lib(\"%(path)s\")
f = lib.load_func(\"return_int_1\", [], \'int)
print(f())
""" % locals(), "-1073741824")

    def test_return300(self):
        path = self.get_lib_path()
        self._test("""
lib = load_lib(\"%(path)s\")
f = lib.load_func(\"return_int_2\", [], \'int)
print(f())
""" % locals(), "1073741823")

    def test_return310(self):
        path = self.get_lib_path()
        self._test("""
lib = load_lib(\"%(path)s\")
f = lib.load_func(\"return_int_3\", [], \'int)
print(f())
""" % locals(), "1073741824")

    def test_return320(self):
        path = self.get_lib_path()
        self._test("""
lib = load_lib(\"%(path)s\")
f = lib.load_func(\"return_ulong_0\", [], \'ulong)
print(f())
""" % locals(), "1073741823")

    def test_return330(self):
        path = self.get_lib_path()
        self._test("""
lib = load_lib(\"%(path)s\")
f = lib.load_func(\"return_ulong_1\", [], \'ulong)
print(f())
""" % locals(), "1073741824")

    def test_return340(self):
        path = self.get_lib_path()
        self._test("""
lib = load_lib(\"%(path)s\")
f = lib.load_func(\"return_long_0\", [], \'long)
prlong(f())
""" % locals(), "-1073741825")

    def test_return350(self):
        path = self.get_lib_path()
        self._test("""
lib = load_lib(\"%(path)s\")
f = lib.load_func(\"return_long_1\", [], \'long)
prlong(f())
""" % locals(), "-1073741824")

    def test_return360(self):
        path = self.get_lib_path()
        self._test("""
lib = load_lib(\"%(path)s\")
f = lib.load_func(\"return_long_2\", [], \'long)
prlong(f())
""" % locals(), "1073741823")

    def test_return370(self):
        path = self.get_lib_path()
        self._test("""
lib = load_lib(\"%(path)s\")
f = lib.load_func(\"return_long_3\", [], \'long)
prlong(f())
""" % locals(), "1073741824")

    def test_return380(self):
        path = self.get_lib_path()
        self._test("""
lib = load_lib(\"%(path)s\")
f = lib.load_func(\"return_long_double\", [], \'longdouble)
print(f())
""" % locals(), "3.14")

    def test_return390(self):
        path = self.get_lib_path()
        self._test("""
lib = load_lib(\"%(path)s\")
f = lib.load_func(\"return_pointer_0\", [], \'pointer)
print(f())
""" % locals(), "1073741823")

    def test_return400(self):
        path = self.get_lib_path()
        self._test("""
lib = load_lib(\"%(path)s\")
f = lib.load_func(\"return_pointer_1\", [], \'pointer)
print(f())
""" % locals(), "1073741824")

# vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
