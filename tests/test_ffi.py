# -*- coding: utf-8 -*-

from os.path import join
from re import match
from testcase import TestCase, get_lib_path
import os

class TestFFI(TestCase):

    def test_load_lib0(self):
        path = get_lib_path()
        self._test("load_lib(\"%(path)s\")" % locals())

    def test_load_func0(self):
        path = get_lib_path()
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

    # Tests for String
    def test_Struct620(self):
        self._test("""
Bar = StructClass.new(\"Bar\", [[[\'string, ENCODINGS[\"ascii\"]], \'baz]])
bar = Bar.new()
print(bar.baz)
""", "nil")

    # Test for Array
    def test_Struct630(self):
        self._test("""
Foo = StructClass.new(\"Foo\", [[[\'int, 32], \'bar]])
foo = Foo.new()
foo.bar[0] = 42
print(foo.bar[0])
""", "42")

    def test_Struct640(self):
        self._test("""
Foo = StructClass.new(\"Foo\", [[[\'char, 4], \'bar]])
foo = Foo.new()
foo.bar[0] = 0x66
foo.bar[1] = 0x6f
foo.bar[2] = 0x6f
foo.bar[3] = 0
print(foo.bar.to_s())
""", "foo")

    def test_Struct650(self):
        path = get_lib_path()
        self._test("""
import libc
enable_gc_stress()
lib = load_lib(\"%(path)s\")
f = lib.load_func(\"test_pointer_p2\", [\'pointer_p])
ptr = Pointer.new()
f(ptr)
try
  Foo = StructClass.new(\"Foo\", [[[\'char, 0], \'bar]])
  foo = Foo.new(ptr)
  print(foo.bar[0])
finally
  libc.free(ptr)
end
""" % locals(), "42", options=[])

    # Tests for pointer
    def test_Struct660(self):
        self._test("""
Foo = StructClass.new(\"Foo\", [[\'pointer, \'bar]])
foo = Foo.new()
print(foo.bar)
""", "0")

    def test_Struct670(self):
        self._test("""
Foo = StructClass.new(\"Foo\", [[\'pointer, \'bar]])
foo = Foo.new()
foo.bar = 42
print(foo.ptr)
""", "42")

    # Tests for internal structs
    def test_Struct680(self):
        self._test("""
Foo = StructClass.new(\"Foo\", [[\'int, \'bar]])
Baz = StructClass.new(\"Baz\", [[Foo, \'foo]])
baz = Baz.new()
baz.foo.bar = 42
print(baz.foo.bar)""", "42")

    def test_Struct690(self):
        path = self.get_lib_path()
        self._test("""
Foo = StructClass.new(\"Foo\", [[\'int, \'bar]])
Baz = StructClass.new(\"Baz\", [[Foo, \'foo]])
lib = load_lib(\"./test_Struct690.so\")
f = lib.load_func(\"test_Struct690\", [Baz])
baz = Baz.new()
f(baz)
print(baz.foo.bar)""", "42")

    def test_Struct700(self):
        self._test("""
Foo = StructClass.new(\"Foo\", [[\'int, \'bar]])
Baz = StructClass.new(\"Baz\", [[Foo, \'foo], [\'int, \'quux]])
lib = load_lib(\"./test_Struct700.so\")
f = lib.load_func(\"test_Struct700\", [Baz])
baz = Baz.new()
f(baz)
print(baz.quux)""", "42")

    def test_Struct705(self):
        self._test("""
Foo = StructClass.new(\"Foo\", [[\'int, \'bar]])
Baz = StructClass.new(\"Baz\", [[\'int, \'quux], [Foo, \'foo]])
lib = load_lib(\"./test_Struct705.so\")
f = lib.load_func(\"test_Struct705\", [Baz])
baz = Baz.new()
f(baz)
print(baz.foo.bar)""", "42")

    def test_Struct710(self):
        self._test("""
Foo = StructClass.new(\"Foo\", [[[\'struct, [[\'int, \'bar]]], \'baz]])
foo = Foo.new()
foo.baz.bar = 42
print(foo.baz.bar)""", "42")

    def test_Struct720(self):
        self._test("""
Foo = StructClass.new(\"Foo\", [[[\'struct, [[\'int, \'bar]]], \'baz]])
lib = load_lib(\"./test_Struct720.so\")
f = lib.load_func(\"test_Struct720\", [Foo])
foo = Foo.new()
f(foo)
print(foo.baz.bar)""", "42")

    def test_Struct730(self):
        self._test("""
Foo = StructClass.new(\"Foo\", [
  [[\'struct, [[\'int, \'bar]]], \'baz],
  [\'int, \'quux]])
lib = load_lib(\"./test_Struct730.so\")
f = lib.load_func(\"test_Struct730\", [Foo])
foo = Foo.new()
f(foo)
print(foo.baz.bar)""", "42")

    # Tests for pointers to a struct
    def test_Struct740(self):
        self._test("""
Foo = StructClass.new(\"Foo\", [[\'int, \'bar]])
Baz = StructClass.new(\"Baz\", [[[\'pointer, Foo], \'foo]])
foo = Foo.new()
baz = Baz.new()
baz.foo = foo
lib = load_lib(\"./test_Struct740.so\")
f = lib.load_func(\"test_Struct740\", [Baz])
f(baz)
print(baz.foo.bar)""", "42")

    # Tests for UnionClass
    def test_Union000(self):
        self._test("""
Foo = UnionClass.new(\"Foo\", [[\'int, \'bar]])
foo = Foo.new()
foo.bar = 42
print(foo.bar)""", "42")

    def test_Union010(self):
        self._test("""
Foo = UnionClass.new(\"Foo\", [[\'int, \'bar], [\'int, \'baz]])
foo = Foo.new()
foo.bar = 42
print(foo.baz)""", "42")

    def test_Union020(self):
        self._test("""
Foo = StructClass.new(\"Foo\", [[\'int, \'bar], [\'int, \'baz]])
Quux = UnionClass.new(\"Quux\", [[\'int, \'hoge], [Foo, \'foo]])
quux = Quux.new()
quux.hoge = 42
print(quux.foo.bar)""", "42")

    def test_Union030(self):
        self._test("""
Foo = UnionClass.new(\"Foo\", [[\'int, \'bar], [\'int, \'baz]])
lib = load_lib(\"./test_Union030.so\")
f = lib.load_func(\"test_Union030\", [[\'pointer, Foo]])
foo = Foo.new()
f(foo)
print(foo.baz)""", "42")

    def test_Union040(self):
        self._test("""
Foo = UnionClass.new(\"Foo\", [[\'int, \'bar], [\'int, \'baz]])
lib = load_lib(\"./test_Union040.so\")
f = lib.load_func(\"test_Union040\", [[\'pointer, Foo]])
foo = Foo.new()
f(foo)
print(foo.bar)""", "42")

    def test_Union050(self):
        self._test("""
Foo = StructClass.new(\"Foo\", [[\'int, \'bar], [\'int, \'baz]])
Quux = UnionClass.new(\"Quux\", [[\'int, \'hoge], [Foo, \'foo]])
quux = Quux.new()
lib = load_lib(\"./test_Union050.so\")
f = lib.load_func(\"test_Union050\", [[\'pointer, Quux]])
print(quux.foo.baz)""", "42")

    # Tests for uint8
    def test_argument10(self):
        def test_stderr(stderr):
            assert 0 < stderr.find("ValueError: Value must be greater or equal 0, not -1")
        path = get_lib_path()
        self._test("""
lib = load_lib(\"%(path)s\")
f = lib.load_func(\"print_uint8\", [\'uint8])
f(-1)
""" % locals(), stderr=test_stderr)

    def test_argument20(self):
        path = get_lib_path()
        self._test("""
lib = load_lib(\"%(path)s\")
f = lib.load_func(\"print_uint8\", [\'uint8])
f(0)
""" % locals(), "0")

    def test_argument30(self):
        path = get_lib_path()
        self._test("""
lib = load_lib(\"%(path)s\")
f = lib.load_func(\"print_uint8\", [\'uint8])
f(255)
""" % locals(), "255")

    def test_argument40(self):
        def test_stderr(stderr):
            assert 0 < stderr.find("ValueError: Value must be less or equal 255, not 256")
        path = get_lib_path()
        self._test("""
lib = load_lib(\"%(path)s\")
f = lib.load_func(\"print_uint8\", [\'uint8])
f(256)
""" % locals(), stderr=test_stderr)

    def test_argument50(self):
        def test_stderr(stderr):
            assert 0 < stderr.find("TypeError: Value must be Fixnum, not String")
        path = get_lib_path()
        self._test("""
lib = load_lib(\"%(path)s\")
f = lib.load_func(\"print_uint8\", [\'uint8])
f(\"baz\")
""" % locals(), stderr=test_stderr)

    # Tests for int8
    def test_argument60(self):
        def test_stderr(stderr):
            assert 0 < stderr.find("ValueError: Value must be greater or equal -128, not -129")
        path = get_lib_path()
        self._test("""
lib = load_lib(\"%(path)s\")
f = lib.load_func(\"print_int8\", [\'int8])
f(-129)
""" % locals(), stderr=test_stderr)

    def test_argument70(self):
        path = get_lib_path()
        self._test("""
lib = load_lib(\"%(path)s\")
f = lib.load_func(\"print_int8\", [\'int8])
f(-128)
""" % locals(), "-128")

    def test_argument80(self):
        path = get_lib_path()
        self._test("""
lib = load_lib(\"%(path)s\")
f = lib.load_func(\"print_int8\", [\'int8])
f(127)
""" % locals(), "127")

    def test_argument90(self):
        def test_stderr(stderr):
            assert 0 < stderr.find("ValueError: Value must be less or equal 127, not 128")
        path = get_lib_path()
        self._test("""
lib = load_lib(\"%(path)s\")
f = lib.load_func(\"print_int8\", [\'int8])
f(128)
""" % locals(), stderr=test_stderr)

    def test_argument100(self):
        def test_stderr(stderr):
            assert 0 < stderr.find("TypeError: Value must be Fixnum, not String")
        path = get_lib_path()
        self._test("""
lib = load_lib(\"%(path)s\")
f = lib.load_func(\"print_int8\", [\'int8])
f(\"baz\")
""" % locals(), stderr=test_stderr)

    # Tests for uint16
    def test_argument110(self):
        def test_stderr(stderr):
            assert 0 < stderr.find("ValueError: Value must be greater or equal 0, not -1")
        path = get_lib_path()
        self._test("""
lib = load_lib(\"%(path)s\")
f = lib.load_func(\"print_uint16\", [\'uint16])
f(-1)
""" % locals(), stderr=test_stderr)

    def test_argument120(self):
        path = get_lib_path()
        self._test("""
lib = load_lib(\"%(path)s\")
f = lib.load_func(\"print_uint16\", [\'uint16])
f(0)
""" % locals(), "0")

    def test_argument130(self):
        path = get_lib_path()
        self._test("""
lib = load_lib(\"%(path)s\")
f = lib.load_func(\"print_uint16\", [\'uint16])
f(65535)
""" % locals(), "65535")

    def test_argument140(self):
        def test_stderr(stderr):
            assert 0 < stderr.find("ValueError: Value must be less or equal 65535, not 65536")
        path = get_lib_path()
        self._test("""
lib = load_lib(\"%(path)s\")
f = lib.load_func(\"print_uint16\", [\'uint16])
f(65536)
""" % locals(), stderr=test_stderr)

    def test_argument150(self):
        def test_stderr(stderr):
            assert 0 < stderr.find("TypeError: Value must be Fixnum, not String")
        path = get_lib_path()
        self._test("""
lib = load_lib(\"%(path)s\")
f = lib.load_func(\"print_uint16\", [\'uint16])
f(\"baz\")
""" % locals(), stderr=test_stderr)

    # Tests for int16
    def test_argument160(self):
        def test_stderr(stderr):
            assert 0 < stderr.find("ValueError: Value must be greater or equal -32768, not -32769")
        path = get_lib_path()
        self._test("""
lib = load_lib(\"%(path)s\")
f = lib.load_func(\"print_int16\", [\'int16])
f(-32769)
""" % locals(), stderr=test_stderr)

    def test_argument170(self):
        path = get_lib_path()
        self._test("""
lib = load_lib(\"%(path)s\")
f = lib.load_func(\"print_int16\", [\'int16])
f(-32768)
""" % locals(), "-32768")

    def test_argument180(self):
        path = get_lib_path()
        self._test("""
lib = load_lib(\"%(path)s\")
f = lib.load_func(\"print_int16\", [\'int16])
f(32767)
""" % locals(), "32767")

    def test_argument190(self):
        def test_stderr(stderr):
            assert 0 < stderr.find("ValueError: Value must be less or equal 32767, not 32768")
        path = get_lib_path()
        self._test("""
lib = load_lib(\"%(path)s\")
f = lib.load_func(\"print_int16\", [\'int16])
f(32768)
""" % locals(), stderr=test_stderr)

    def test_argument200(self):
        def test_stderr(stderr):
            assert 0 < stderr.find("TypeError: Value must be Fixnum, not String")
        path = get_lib_path()
        self._test("""
lib = load_lib(\"%(path)s\")
f = lib.load_func(\"print_int16\", [\'int16])
f(\"baz\")
""" % locals(), stderr=test_stderr)

    # Tests for uint32
    def test_argument210(self):
        def test_stderr(stderr):
            assert 0 < stderr.find("ValueError: Value must be greater or equal 0, not -1")
        path = get_lib_path()
        self._test("""
lib = load_lib(\"%(path)s\")
f = lib.load_func(\"print_uint32\", [\'uint32])
f(-1)
""" % locals(), stderr=test_stderr)

    def test_argument220(self):
        path = get_lib_path()
        self._test("""
lib = load_lib(\"%(path)s\")
f = lib.load_func(\"print_uint32\", [\'uint32])
f(0)
""" % locals(), "0")

    def test_argument230(self):
        path = get_lib_path()
        self._test("""
lib = load_lib(\"%(path)s\")
f = lib.load_func(\"print_uint32\", [\'uint32])
f(4294967295)
""" % locals(), "4294967295")

    def test_argument240(self):
        def test_stderr(stderr):
            assert 0 < stderr.find("ValueError: Value must be less or equal 4294967295, not 4294967296")
        path = get_lib_path()
        self._test("""
lib = load_lib(\"%(path)s\")
f = lib.load_func(\"print_uint32\", [\'uint32])
f(4294967296)
""" % locals(), stderr=test_stderr)

    def test_argument250(self):
        def test_stderr(stderr):
            assert 0 < stderr.find("TypeError: Value must be Fixnum or Bignum, not String")
        path = get_lib_path()
        self._test("""
lib = load_lib(\"%(path)s\")
f = lib.load_func(\"print_uint32\", [\'uint32])
f(\"baz\")
""" % locals(), stderr=test_stderr)

    # Tests for int32
    def test_argument260(self):
        def test_stderr(stderr):
            assert 0 < stderr.find("ValueError: Value must be greater or equal -2147483648, not -2147483649")
        path = get_lib_path()
        self._test("""
lib = load_lib(\"%(path)s\")
f = lib.load_func(\"print_int32\", [\'int32])
f(-2147483649)
""" % locals(), stderr=test_stderr)

    def test_argument270(self):
        path = get_lib_path()
        self._test("""
lib = load_lib(\"%(path)s\")
f = lib.load_func(\"print_int32\", [\'int32])
f(-2147483648)
""" % locals(), "-2147483648")

    def test_argument280(self):
        path = get_lib_path()
        self._test("""
lib = load_lib(\"%(path)s\")
f = lib.load_func(\"print_int32\", [\'int32])
f(2147483647)
""" % locals(), "2147483647")

    def test_argument290(self):
        def test_stderr(stderr):
            assert 0 < stderr.find("ValueError: Value must be less or equal 2147483647, not 2147483648")
        path = get_lib_path()
        self._test("""
lib = load_lib(\"%(path)s\")
f = lib.load_func(\"print_int32\", [\'int32])
f(2147483648)
""" % locals(), stderr=test_stderr)

    def test_argument300(self):
        def test_stderr(stderr):
            assert 0 < stderr.find("TypeError: Value must be Fixnum or Bignum, not String")
        path = get_lib_path()
        self._test("""
lib = load_lib(\"%(path)s\")
f = lib.load_func(\"print_int32\", [\'int32])
f(\"baz\")
""" % locals(), stderr=test_stderr)

    # Tests for int32
    def test_argument310(self):
        def test_stderr(stderr):
            assert 0 < stderr.find("ValueError: Value must be greater or equal -2147483648, not -2147483649")
        path = get_lib_path()
        self._test("""
lib = load_lib(\"%(path)s\")
f = lib.load_func(\"print_int32\", [\'int32])
f(-2147483649)
""" % locals(), stderr=test_stderr)

    def test_argument320(self):
        path = get_lib_path()
        self._test("""
lib = load_lib(\"%(path)s\")
f = lib.load_func(\"print_int32\", [\'int32])
f(-2147483648)
""" % locals(), "-2147483648")

    def test_argument330(self):
        path = get_lib_path()
        self._test("""
lib = load_lib(\"%(path)s\")
f = lib.load_func(\"print_int32\", [\'int32])
f(2147483647)
""" % locals(), "2147483647")

    def test_argument340(self):
        def test_stderr(stderr):
            assert 0 < stderr.find("ValueError: Value must be less or equal 2147483647, not 2147483648")
        path = get_lib_path()
        self._test("""
lib = load_lib(\"%(path)s\")
f = lib.load_func(\"print_int32\", [\'int32])
f(2147483648)
""" % locals(), stderr=test_stderr)

    def test_argument350(self):
        def test_stderr(stderr):
            assert 0 < stderr.find("TypeError: Value must be Fixnum or Bignum, not String")
        path = get_lib_path()
        self._test("""
lib = load_lib(\"%(path)s\")
f = lib.load_func(\"print_int32\", [\'int32])
f(\"baz\")
""" % locals(), stderr=test_stderr)

    # Tests for float
    def test_argument460(self):
        path = get_lib_path()
        self._test("""
lib = load_lib(\"%(path)s\")
f = lib.load_func(\"print_float\", [\'float])
f(3.14)
""" % locals(), "3.14")

    def test_argument470(self):
        def test_stderr(stderr):
            assert 0 < stderr.find("TypeError: Value must be Float, not String")
        path = get_lib_path()
        self._test("""
lib = load_lib(\"%(path)s\")
f = lib.load_func(\"print_float\", [\'float])
f(\"baz\")
""" % locals(), stderr=test_stderr)

    # Tests for double
    def test_argument480(self):
        path = get_lib_path()
        self._test("""
lib = load_lib(\"%(path)s\")
f = lib.load_func(\"print_double\", [\'double])
f(3.14)
""" % locals(), "3.14")

    def test_argument490(self):
        def test_stderr(stderr):
            assert 0 < stderr.find("TypeError: Value must be Float, not String")
        path = get_lib_path()
        self._test("""
lib = load_lib(\"%(path)s\")
f = lib.load_func(\"print_double\", [\'double])
f(\"baz\")
""" % locals(), stderr=test_stderr)

    # Tests for uchar
    def test_argument500(self):
        def test_stderr(stderr):
            assert 0 < stderr.find("ValueError: Value must be greater or equal 0, not -1")
        path = get_lib_path()
        self._test("""
lib = load_lib(\"%(path)s\")
f = lib.load_func(\"print_uchar\", [\'uchar])
f(-1)
""" % locals(), stderr=test_stderr)

    def test_argument510(self):
        path = get_lib_path()
        self._test("""
lib = load_lib(\"%(path)s\")
f = lib.load_func(\"print_uchar\", [\'uchar])
f(0)
""" % locals(), "0")

    def test_argument520(self):
        path = get_lib_path()
        self._test("""
lib = load_lib(\"%(path)s\")
f = lib.load_func(\"print_uchar\", [\'uchar])
f(255)
""" % locals(), "255")

    def test_argument530(self):
        def test_stderr(stderr):
            assert 0 < stderr.find("ValueError: Value must be less or equal 255, not 256")
        path = get_lib_path()
        self._test("""
lib = load_lib(\"%(path)s\")
f = lib.load_func(\"print_uchar\", [\'uchar])
f(256)
""" % locals(), stderr=test_stderr)

    def test_argument540(self):
        def test_stderr(stderr):
            assert 0 < stderr.find("TypeError: Value must be Fixnum, not String")
        path = get_lib_path()
        self._test("""
lib = load_lib(\"%(path)s\")
f = lib.load_func(\"print_uchar\", [\'uchar])
f(\"baz\")
""" % locals(), stderr=test_stderr)

    # Tests for char
    def test_argument550(self):
        def test_stderr(stderr):
            assert 0 < stderr.find("ValueError: Value must be greater or equal -128, not -129")
        path = get_lib_path()
        self._test("""
lib = load_lib(\"%(path)s\")
f = lib.load_func(\"print_char\", [\'char])
f(-129)
""" % locals(), stderr=test_stderr)

    def test_argument560(self):
        path = get_lib_path()
        self._test("""
lib = load_lib(\"%(path)s\")
f = lib.load_func(\"print_char\", [\'char])
f(-128)
""" % locals(), "-128")

    def test_argument570(self):
        path = get_lib_path()
        self._test("""
lib = load_lib(\"%(path)s\")
f = lib.load_func(\"print_char\", [\'char])
f(127)
""" % locals(), "127")

    def test_argument580(self):
        def test_stderr(stderr):
            assert 0 < stderr.find("ValueError: Value must be less or equal 127, not 128")
        path = get_lib_path()
        self._test("""
lib = load_lib(\"%(path)s\")
f = lib.load_func(\"print_char\", [\'char])
f(128)
""" % locals(), stderr=test_stderr)

    def test_argument590(self):
        def test_stderr(stderr):
            assert 0 < stderr.find("TypeError: Value must be Fixnum, not String")
        path = get_lib_path()
        self._test("""
lib = load_lib(\"%(path)s\")
f = lib.load_func(\"print_char\", [\'char])
f(\"baz\")
""" % locals(), stderr=test_stderr)

    def test_argument620(self):
        path = get_lib_path()
        self._test("""
Foo = StructClass.new(\"Foo\", [['int, 'bar]])
foo = Foo.new()
foo.bar = 42
lib = load_lib(\"%(path)s\")
f = lib.load_func(\"print_struct\", [Foo])
f(foo)
""" % locals(), "42")

    def test_argument630(self):
        path = get_lib_path()
        self._test("""
Bar = StructClass.new(\"Bar\", [[[\'string, ENCODINGS[\"ascii\"]], \'baz]])
bar = Bar.new()
lib = load_lib(\"%(path)s\")
f = lib.load_func(\"test_string\", [Bar])
f(bar)
print(bar.baz)
""" % locals(), "foobarbazquux")

    # Tests for pointer
    def test_argument640(self):
        def test_stderr(stderr):
            assert 0 < stderr.find("TypeError: Argument must be Pointer, not Fixnum")
        path = get_lib_path()
        self._test("""
lib = load_lib(\"%(path)s\")
f = lib.load_func(\"print_pointer\", [\'pointer])
f(42)
""" % locals(), stderr=test_stderr)

    def test_argument650(self):
        path = get_lib_path()
        self._test("""
lib = load_lib(\"%(path)s\")
f = lib.load_func(\"print_pointer\", [\'pointer])
f(Pointer.new())
""" % locals(), "NULL")

    def test_argument660(self):
        path = get_lib_path()
        self._test("""
lib = load_lib(\"%(path)s\")
f = lib.load_func(\"print_string\", [[\'string, ENCODINGS[\"ascii\"]]])
f(\"foo\")
""" % locals(), "foo")

    def test_arguments0(self):
        path = get_lib_path()
        self._test("""
lib = load_lib(\"%(path)s\")
f = lib.load_func(\"print_two_int\", [\'int, \'int])
f(42, 26)
""" % locals(), "4226")

    def test_return0(self):
        path = get_lib_path()
        self._test("""
lib = load_lib(\"%(path)s\")
f = lib.load_func(\"return_uint8\", [], \'uint8)
print(f())
""" % locals(), "42")

    def test_return10(self):
        path = get_lib_path()
        self._test("""
lib = load_lib(\"%(path)s\")
f = lib.load_func(\"return_int8\", [], \'int8)
print(f())
""" % locals(), "42")

    def test_return20(self):
        path = get_lib_path()
        self._test("""
lib = load_lib(\"%(path)s\")
f = lib.load_func(\"return_uint16\", [], \'uint16)
print(f())
""" % locals(), "42")

    def test_return30(self):
        path = get_lib_path()
        self._test("""
lib = load_lib(\"%(path)s\")
f = lib.load_func(\"return_int16\", [], \'int16)
print(f())
""" % locals(), "42")

    def test_return40(self):
        path = get_lib_path()
        self._test("""
lib = load_lib(\"%(path)s\")
f = lib.load_func(\"return_uint32_0\", [], \'uint32)
print(f())
""" % locals(), "1073741823")

    def test_return50(self):
        path = get_lib_path()
        self._test("""
lib = load_lib(\"%(path)s\")
f = lib.load_func(\"return_uint32_1\", [], \'uint32)
print(f())
""" % locals(), "1073741824")

    def test_return60(self):
        path = get_lib_path()
        self._test("""
lib = load_lib(\"%(path)s\")
f = lib.load_func(\"return_int32_0\", [], \'int32)
print(f())
""" % locals(), "-1073741825")

    def test_return70(self):
        path = get_lib_path()
        self._test("""
lib = load_lib(\"%(path)s\")
f = lib.load_func(\"return_int32_1\", [], \'int32)
print(f())
""" % locals(), "-1073741824")

    def test_return80(self):
        path = get_lib_path()
        self._test("""
lib = load_lib(\"%(path)s\")
f = lib.load_func(\"return_int32_2\", [], \'int32)
print(f())
""" % locals(), "1073741823")

    def test_return90(self):
        path = get_lib_path()
        self._test("""
lib = load_lib(\"%(path)s\")
f = lib.load_func(\"return_int32_3\", [], \'int32)
print(f())
""" % locals(), "1073741824")

    def test_return200(self):
        path = get_lib_path()
        self._test("""
lib = load_lib(\"%(path)s\")
f = lib.load_func(\"return_float\", [], \'float)
print(f())
""" % locals(), "3.1400001049")

    def test_return210(self):
        path = get_lib_path()
        self._test("""
lib = load_lib(\"%(path)s\")
f = lib.load_func(\"return_double\", [], \'double)
print(f())
""" % locals(), "3.14")

    def test_return220(self):
        path = get_lib_path()
        self._test("""
lib = load_lib(\"%(path)s\")
f = lib.load_func(\"return_uchar\", [], \'uchar)
print(f())
""" % locals(), "42")

    def test_return230(self):
        path = get_lib_path()
        self._test("""
lib = load_lib(\"%(path)s\")
f = lib.load_func(\"return_char\", [], \'char)
print(f())
""" % locals(), "42")

    def test_return240(self):
        path = get_lib_path()
        self._test("""
lib = load_lib(\"%(path)s\")
f = lib.load_func(\"return_ushort\", [], \'ushort)
print(f())
""" % locals(), "42")

    def test_return250(self):
        path = get_lib_path()
        self._test("""
lib = load_lib(\"%(path)s\")
f = lib.load_func(\"return_short\", [], \'short)
print(f())
""" % locals(), "42")

    def test_return260(self):
        path = get_lib_path()
        self._test("""
lib = load_lib(\"%(path)s\")
f = lib.load_func(\"return_uint_0\", [], \'uint)
print(f())
""" % locals(), "1073741823")

    def test_return270(self):
        path = get_lib_path()
        self._test("""
lib = load_lib(\"%(path)s\")
f = lib.load_func(\"return_uint_1\", [], \'uint)
print(f())
""" % locals(), "1073741824")

    def test_return280(self):
        path = get_lib_path()
        self._test("""
lib = load_lib(\"%(path)s\")
f = lib.load_func(\"return_int_0\", [], \'int)
print(f())
""" % locals(), "-1073741825")

    def test_return290(self):
        path = get_lib_path()
        self._test("""
lib = load_lib(\"%(path)s\")
f = lib.load_func(\"return_int_1\", [], \'int)
print(f())
""" % locals(), "-1073741824")

    def test_return300(self):
        path = get_lib_path()
        self._test("""
lib = load_lib(\"%(path)s\")
f = lib.load_func(\"return_int_2\", [], \'int)
print(f())
""" % locals(), "1073741823")

    def test_return310(self):
        path = get_lib_path()
        self._test("""
lib = load_lib(\"%(path)s\")
f = lib.load_func(\"return_int_3\", [], \'int)
print(f())
""" % locals(), "1073741824")

    def test_return320(self):
        path = get_lib_path()
        self._test("""
lib = load_lib(\"%(path)s\")
f = lib.load_func(\"return_ulong_0\", [], \'ulong)
print(f())
""" % locals(), "1073741823")

    def test_return330(self):
        path = get_lib_path()
        self._test("""
lib = load_lib(\"%(path)s\")
f = lib.load_func(\"return_ulong_1\", [], \'ulong)
print(f())
""" % locals(), "1073741824")

    def test_return340(self):
        path = get_lib_path()
        self._test("""
lib = load_lib(\"%(path)s\")
f = lib.load_func(\"return_long_0\", [], \'long)
print(f())
""" % locals(), "-1073741825")

    def test_return350(self):
        path = get_lib_path()
        self._test("""
lib = load_lib(\"%(path)s\")
f = lib.load_func(\"return_long_1\", [], \'long)
print(f())
""" % locals(), "-1073741824")

    def test_return360(self):
        path = get_lib_path()
        self._test("""
lib = load_lib(\"%(path)s\")
f = lib.load_func(\"return_long_2\", [], \'long)
print(f())
""" % locals(), "1073741823")

    def test_return370(self):
        path = get_lib_path()
        self._test("""
lib = load_lib(\"%(path)s\")
f = lib.load_func(\"return_long_3\", [], \'long)
print(f())
""" % locals(), "1073741824")

    def test_return380(self):
        path = get_lib_path()
        self._test("""
lib = load_lib(\"%(path)s\")
f = lib.load_func(\"return_long_double\", [], \'longdouble)
print(f())
""" % locals(), "3.14")

    def test_return390(self):
        def test_stdout(stdout):
            assert match(r"<Pointer .*>\Z", stdout) is not None
        path = get_lib_path()
        self._test("""
lib = load_lib(\"%(path)s\")
f = lib.load_func(\"return_pointer_0\", [], \'pointer)
print(f())
""" % locals(), test_stdout)

    def test_Int0(self):
        self._test("print(Int.new().value)", "0")

    def test_Int10(self):
        path = get_lib_path()
        self._test("""
lib = load_lib(\"%(path)s\")
f = lib.load_func(\"test_int_p\", ['int_p])
int = Int.new()
f(int)
print(int.value)
""" % locals(), "42")

    def test_Pointer0(self):
        path = get_lib_path()
        self._test("""
import libc
enable_gc_stress()
lib = load_lib(\"%(path)s\")
f = lib.load_func(\"test_pointer_p\", [\'pointer_p])
ptr = Pointer.new()
f(ptr)
try
  Foo = StructClass.new(\"Foo\", [[\'int, \'bar]])
  foo = Foo.new(ptr)
  print(foo.bar)
finally
  libc.free(ptr)
end
""" % locals(), "42", options=[])

    def test_Buffer0(self):
        self._test("""
buf = Buffer.new(42)
print(buf.size)
""", "42")

    def test_Buffer10(self):
        self._test("""
s = \"foo\"
buf = Buffer.new(s.to_bin(ENCODINGS[\"utf-8\"]))
Bar = StructClass.new(\"Bar\", [[Buffer, \'baz]])
bar = Bar.new()
bar.baz = buf
print(bar.baz.to_s(buf.size, ENCODINGS[\"utf-8\"]))
""", "foo")

    def test_Buffer20(self):
        path = get_lib_path()
        self._test("""
s = \"foo\"
buf = Buffer.new(s.to_bin(ENCODINGS[\"utf-8\"]))
Bar = StructClass.new(\"Bar\", [[Buffer, \'baz]])
bar = Bar.new()
bar.baz = buf
lib = load_lib(\"%(path)s\")
f = lib.load_func(\"test_Buffer0\", [Bar])
f(bar)
print(bar.baz.to_s(buf.size, ENCODINGS[\"utf-8\"]))
""" % locals(), "bar")

# vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
