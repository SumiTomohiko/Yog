# -*- coding: utf-8 -*-

from testcase import TestCase
from utils import is_32bit

class TestFFI(TestCase):

    disabled = is_32bit() is not True

    # Tests for ushort
    def test_Struct0(self):
        def test_stderr(stderr):
            assert 0 < stderr.find("Value must be greater or equal 0, not -1")
        self._test("""
Foo = StructClass.new(\"Foo\", [[\'ushort, \'bar]])
foo = Foo.new()
foo.bar = -1
""", stderr=test_stderr)

    def test_Struct10(self):
        self._test("""
Foo = StructClass.new(\"Foo\", [[\'ushort, \'bar]])
foo = Foo.new()
foo.bar = 0
print(foo.bar)
""", "0")

    def test_Struct20(self):
        self._test("""
Foo = StructClass.new(\"Foo\", [[\'ushort, \'bar]])
foo = Foo.new()
foo.bar = 65535
print(foo.bar)
""", "65535")

    def test_Struct30(self):
        def test_stderr(stderr):
            assert 0 < stderr.find("Value must be less or equal 65535, not 65536")
        self._test("""
Foo = StructClass.new(\"Foo\", [[\'ushort, \'bar]])
foo = Foo.new()
foo.bar = 65536
""", stderr=test_stderr)

    def test_Struct40(self):
        def test_stderr(stderr):
            assert 0 < stderr.find("TypeError: Value must be Fixnum, not String")
        self._test("""
Foo = StructClass.new(\"Foo\", [[\'ushort, \'bar]])
foo = Foo.new()
foo.bar = \"baz\"
""", stderr=test_stderr)

    # Tests for short
    def test_Struct50(self):
        def test_stderr(stderr):
            assert 0 < stderr.find("Value must be greater or equal -32768, not -32769")
        self._test("""
Foo = StructClass.new(\"Foo\", [[\'short, \'bar]])
foo = Foo.new()
foo.bar = -32769
""", stderr=test_stderr)

    def test_Struct60(self):
        self._test("""
Foo = StructClass.new(\"Foo\", [[\'short, \'bar]])
foo = Foo.new()
foo.bar = -32768
print(foo.bar)
""", "-32768")

    def test_Struct70(self):
        self._test("""
Foo = StructClass.new(\"Foo\", [[\'short, \'bar]])
foo = Foo.new()
foo.bar = 32767
print(foo.bar)
""", "32767")

    def test_Struct80(self):
        def test_stderr(stderr):
            assert 0 < stderr.find("Value must be less or equal 32767, not 32768")
        self._test("""
Foo = StructClass.new(\"Foo\", [[\'short, \'bar]])
foo = Foo.new()
foo.bar = 32768
""", stderr=test_stderr)

    def test_Struct90(self):
        def test_stderr(stderr):
            assert 0 < stderr.find("TypeError: Value must be Fixnum, not String")
        self._test("""
Foo = StructClass.new(\"Foo\", [[\'short, \'bar]])
foo = Foo.new()
foo.bar = \"baz\"
""", stderr=test_stderr)

    # Tests for uint
    def test_Struct100(self):
        def test_stderr(stderr):
            assert 0 < stderr.find("Value must be greater or equal 0, not -1")
        self._test("""
Foo = StructClass.new(\"Foo\", [[\'uint, \'bar]])
foo = Foo.new()
foo.bar = -1
""", stderr=test_stderr)

    def test_Struct110(self):
        self._test("""
Foo = StructClass.new(\"Foo\", [[\'uint, \'bar]])
foo = Foo.new()
foo.bar = 0
print(foo.bar)
""", "0")

    def test_Struct120(self):
        self._test("""
Foo = StructClass.new(\"Foo\", [[\'uint, \'bar]])
foo = Foo.new()
foo.bar = 4294967295
print(foo.bar)
""", "4294967295")

    def test_Struct130(self):
        def test_stderr(stderr):
            assert 0 < stderr.find("Value must be less or equal 4294967295, not 4294967296")
        self._test("""
Foo = StructClass.new(\"Foo\", [[\'uint, \'bar]])
foo = Foo.new()
foo.bar = 4294967296
""", stderr=test_stderr)

    def test_Struct140(self):
        def test_stderr(stderr):
            assert 0 < stderr.find("TypeError: Value must be Fixnum or Bignum, not String")
        self._test("""
Foo = StructClass.new(\"Foo\", [[\'uint, \'bar]])
foo = Foo.new()
foo.bar = \"baz\"
""", stderr=test_stderr)

    # Tests for int
    def test_Struct150(self):
        def test_stderr(stderr):
            assert 0 < stderr.find("Value must be greater or equal -2147483648, not -2147483649")
        self._test("""
Foo = StructClass.new(\"Foo\", [[\'int, \'bar]])
foo = Foo.new()
foo.bar = -2147483649
""", stderr=test_stderr)

    def test_Struct160(self):
        self._test("""
Foo = StructClass.new(\"Foo\", [[\'int, \'bar]])
foo = Foo.new()
foo.bar = -2147483648
print(foo.bar)
""", "-2147483648")

    def test_Struct170(self):
        self._test("""
Foo = StructClass.new(\"Foo\", [[\'int, \'bar]])
foo = Foo.new()
foo.bar = 2147483647
print(foo.bar)
""", "2147483647")

    def test_Struct180(self):
        def test_stderr(stderr):
            assert 0 < stderr.find("Value must be less or equal 2147483647, not 2147483648")
        self._test("""
Foo = StructClass.new(\"Foo\", [[\'int, \'bar]])
foo = Foo.new()
foo.bar = 2147483648
""", stderr=test_stderr)

    def test_Struct190(self):
        def test_stderr(stderr):
            assert 0 < stderr.find("TypeError: Value must be Fixnum or Bignum, not String")
        self._test("""
Foo = StructClass.new(\"Foo\", [[\'int, \'bar]])
foo = Foo.new()
foo.bar = \"baz\"
""", stderr=test_stderr)

    # Tests for ulong
    def test_Struct200(self):
        def test_stderr(stderr):
            assert 0 < stderr.find("Value must be greater or equal 0, not -1")
        self._test("""
Foo = StructClass.new(\"Foo\", [[\'ulong, \'bar]])
foo = Foo.new()
foo.bar = -1
""", stderr=test_stderr)

    def test_Struct210(self):
        self._test("""
Foo = StructClass.new(\"Foo\", [[\'ulong, \'bar]])
foo = Foo.new()
foo.bar = 0
print(foo.bar)
""", "0")

    def test_Struct220(self):
        self._test("""
Foo = StructClass.new(\"Foo\", [[\'ulong, \'bar]])
foo = Foo.new()
foo.bar = 4294967295
print(foo.bar)
""", "4294967295")

    def test_Struct230(self):
        def test_stderr(stderr):
            assert 0 < stderr.find("Value must be less or equal 4294967295, not 4294967296")
        self._test("""
Foo = StructClass.new(\"Foo\", [[\'ulong, \'bar]])
foo = Foo.new()
foo.bar = 4294967296
""", stderr=test_stderr)

    def test_Struct240(self):
        def test_stderr(stderr):
            assert 0 < stderr.find("TypeError: Value must be Fixnum or Bignum, not String")
        self._test("""
Foo = StructClass.new(\"Foo\", [[\'ulong, \'bar]])
foo = Foo.new()
foo.bar = \"baz\"
""", stderr=test_stderr)

    # Tests for long
    def test_Struct250(self):
        def test_stderr(stderr):
            assert 0 < stderr.find("Value must be greater or equal -2147483648, not -2147483649")
        self._test("""
Foo = StructClass.new(\"Foo\", [[\'long, \'bar]])
foo = Foo.new()
foo.bar = -2147483649
""", stderr=test_stderr)

    def test_Struct260(self):
        self._test("""
Foo = StructClass.new(\"Foo\", [[\'long, \'bar]])
foo = Foo.new()
foo.bar = -2147483648
print(foo.bar)
""", "-2147483648")

    def test_Struct270(self):
        self._test("""
Foo = StructClass.new(\"Foo\", [[\'long, \'bar]])
foo = Foo.new()
foo.bar = 2147483647
print(foo.bar)
""", "2147483647")

    def test_Struct280(self):
        def test_stderr(stderr):
            assert 0 < stderr.find("Value must be less or equal 2147483647, not 2147483648")
        self._test("""
Foo = StructClass.new(\"Foo\", [[\'long, \'bar]])
foo = Foo.new()
foo.bar = 2147483648
""", stderr=test_stderr)

    def test_Struct290(self):
        def test_stderr(stderr):
            assert 0 < stderr.find("TypeError: Value must be Fixnum or Bignum, not String")
        self._test("""
Foo = StructClass.new(\"Foo\", [[\'long, \'bar]])
foo = Foo.new()
foo.bar = \"baz\"
""", stderr=test_stderr)

# vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
