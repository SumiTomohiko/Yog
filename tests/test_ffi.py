# -*- coding: utf-8 -*-

from os.path import join
from testcase import TestCase
import os

class TestFFI(TestCase):

    def get_lib_path(self):
        return join(".", "foo" + ".so" if os.name == "posix" else ".dll")

    def test_load_lib0(self):
        path = self.get_lib_path()
        self._test("load_lib(\"%(path)s\")" % locals())

    def test_find_func0(self):
        path = self.get_lib_path()
        self._test("""
lib = load_lib(\"%(path)s\")
f = lib.load_func(\"foo\")
f()
""" % locals(), "42")

    def test_Struct0(self):
        self._test("""
st = StructClass.new(\"Foo\", [[\'int, \'bar]])
o = st.new()
o.bar = 42
print(o.bar)
""", "42")

    # Tests for uint8
    def test_Struct10(self):
        def test_stderr(stderr):
            assert 0 < stderr.find("ValueError: Value must be greater or equal zero, not -1")
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
            assert 0 < stderr.find("ValueError: Value exceeds range of uint8")
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
            assert 0 < stderr.find("ValueError: Value exceeds range of int8")
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
            assert 0 < stderr.find("ValueError: Value exceeds range of int8")
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
            assert 0 < stderr.find("ValueError: Value exceeds range of uint16")
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
            assert 0 < stderr.find("ValueError: Value exceeds range of uint16")
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
            assert 0 < stderr.find("ValueError: Value exceeds range of int16")
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
            assert 0 < stderr.find("ValueError: Value exceeds range of int16")
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
            assert 0 < stderr.find("ValueError: Value must be greater or equal zero, not -1")
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

    # Tests for int32
    def test_Struct260(self):
        def test_stderr(stderr):
            assert 0 < stderr.find("ValueError: Value must be greater or equal -2147483648, not -2147483649")
        self._test("""
Foo = StructClass.new(\"Foo\", [[\'int32, \'bar]])
foo = Foo.new()
foo.bar = -2147483649
""", stderr=test_stderr)

    def test_Struct270(self):
        self._test("""
Foo = StructClass.new(\"Foo\", [[\'int32, \'bar]])
foo = Foo.new()
foo.bar = -2147483648
print(foo.bar)
""", "-2147483648")

    def test_Struct280(self):
        self._test("""
Foo = StructClass.new(\"Foo\", [[\'int32, \'bar]])
foo = Foo.new()
foo.bar = 2147483647
print(foo.bar)
""", "2147483647")

    def test_Struct290(self):
        def test_stderr(stderr):
            assert 0 < stderr.find("ValueError: Value must be less or equal 2147483647, not 2147483648")
        self._test("""
Foo = StructClass.new(\"Foo\", [[\'int32, \'bar]])
foo = Foo.new()
foo.bar = 2147483648
""", stderr=test_stderr)

    def test_Struct300(self):
        def test_stderr(stderr):
            assert 0 < stderr.find("TypeError: Value must be Fixnum or Bignum, not String")
        self._test("""
Foo = StructClass.new(\"Foo\", [[\'int32, \'bar]])
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
            assert 0 < stderr.find("ValueError: Value must be greater or equal zero, not -1")
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
""", "3.14")

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
            assert 0 < stderr.find("ValueError: Value exceeds range of uchar")
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
            assert 0 < stderr.find("ValueError: Value exceeds range of uchar")
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

    # Tests for schar
    def test_Struct550(self):
        def test_stderr(stderr):
            assert 0 < stderr.find("ValueError: Value exceeds range of schar")
        self._test("""
Foo = StructClass.new(\"Foo\", [[\'schar, \'bar]])
foo = Foo.new()
foo.bar = -129
""", stderr=test_stderr)

    def test_Struct560(self):
        self._test("""
Foo = StructClass.new(\"Foo\", [[\'schar, \'bar]])
foo = Foo.new()
foo.bar = -128
print(foo.bar)
""", "-128")

    def test_Struct570(self):
        self._test("""
Foo = StructClass.new(\"Foo\", [[\'schar, \'bar]])
foo = Foo.new()
foo.bar = 127
print(foo.bar)
""", "127")

    def test_Struct580(self):
        def test_stderr(stderr):
            assert 0 < stderr.find("ValueError: Value exceeds range of schar")
        self._test("""
Foo = StructClass.new(\"Foo\", [[\'schar, \'bar]])
foo = Foo.new()
foo.bar = 128
""", stderr=test_stderr)

    def test_Struct590(self):
        def test_stderr(stderr):
            assert 0 < stderr.find("TypeError: Value must be Fixnum, not String")
        self._test("""
Foo = StructClass.new(\"Foo\", [[\'schar, \'bar]])
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

# vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
