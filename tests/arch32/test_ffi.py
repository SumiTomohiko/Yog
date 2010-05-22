# -*- coding: utf-8 -*-

from testcase import TestCase
from utils import is_32bit

class TestFFI(TestCase):

    disabled = is_32bit() is not True

    # Tests for ushort
    def test_Struct0(self):
        def test_stderr(stderr):
            assert 0 < stderr.find("ValueError: Value exceeds range of ushort")
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
            assert 0 < stderr.find("ValueError: Value exceeds range of ushort")
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

    # Tests for sshort
    def test_Struct50(self):
        def test_stderr(stderr):
            assert 0 < stderr.find("ValueError: Value exceeds range of sshort")
        self._test("""
Foo = StructClass.new(\"Foo\", [[\'sshort, \'bar]])
foo = Foo.new()
foo.bar = -32769
""", stderr=test_stderr)

    def test_Struct60(self):
        self._test("""
Foo = StructClass.new(\"Foo\", [[\'sshort, \'bar]])
foo = Foo.new()
foo.bar = -32768
print(foo.bar)
""", "-32768")

    def test_Struct70(self):
        self._test("""
Foo = StructClass.new(\"Foo\", [[\'sshort, \'bar]])
foo = Foo.new()
foo.bar = 32767
print(foo.bar)
""", "32767")

    def test_Struct80(self):
        def test_stderr(stderr):
            assert 0 < stderr.find("ValueError: Value exceeds range of sshort")
        self._test("""
Foo = StructClass.new(\"Foo\", [[\'sshort, \'bar]])
foo = Foo.new()
foo.bar = 32768
""", stderr=test_stderr)

    def test_Struct90(self):
        def test_stderr(stderr):
            assert 0 < stderr.find("TypeError: Value must be Fixnum, not String")
        self._test("""
Foo = StructClass.new(\"Foo\", [[\'sshort, \'bar]])
foo = Foo.new()
foo.bar = \"baz\"
""", stderr=test_stderr)

    # Tests for uint
    def test_Struct100(self):
        def test_stderr(stderr):
            assert 0 < stderr.find("ValueError: Value exceeds range of uint")
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
            assert 0 < stderr.find("ValueError: Value exceeds range of uint")
        self._test("""
Foo = StructClass.new(\"Foo\", [[\'uint, \'bar]])
foo = Foo.new()
foo.bar = 4294967296
""", stderr=test_stderr)

    def test_Struct140(self):
        def test_stderr(stderr):
            assert 0 < stderr.find("TypeError: Value must be Fixnum, not String")
        self._test("""
Foo = StructClass.new(\"Foo\", [[\'uint, \'bar]])
foo = Foo.new()
foo.bar = \"baz\"
""", stderr=test_stderr)

    # Tests for sint
    def test_Struct150(self):
        def test_stderr(stderr):
            assert 0 < stderr.find("ValueError: Value exceeds range of sint")
        self._test("""
Foo = StructClass.new(\"Foo\", [[\'sint, \'bar]])
foo = Foo.new()
foo.bar = -2147483649
""", stderr=test_stderr)

    def test_Struct160(self):
        self._test("""
Foo = StructClass.new(\"Foo\", [[\'sint, \'bar]])
foo = Foo.new()
foo.bar = -2147483648
print(foo.bar)
""", "-2147483648")

    def test_Struct170(self):
        self._test("""
Foo = StructClass.new(\"Foo\", [[\'sint, \'bar]])
foo = Foo.new()
foo.bar = 2147483647
print(foo.bar)
""", "2147483647")

    def test_Struct180(self):
        def test_stderr(stderr):
            assert 0 < stderr.find("ValueError: Value exceeds range of sint")
        self._test("""
Foo = StructClass.new(\"Foo\", [[\'sint, \'bar]])
foo = Foo.new()
foo.bar = 2147483648
""", stderr=test_stderr)

    def test_Struct190(self):
        def test_stderr(stderr):
            assert 0 < stderr.find("TypeError: Value must be Fixnum, not String")
        self._test("""
Foo = StructClass.new(\"Foo\", [[\'sint, \'bar]])
foo = Foo.new()
foo.bar = \"baz\"
""", stderr=test_stderr)

    # Tests for ulong
    def test_Struct200(self):
        def test_stderr(stderr):
            assert 0 < stderr.find("ValueError: Value exceeds range of ulong")
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
            assert 0 < stderr.find("ValueError: Value exceeds range of ulong")
        self._test("""
Foo = StructClass.new(\"Foo\", [[\'ulong, \'bar]])
foo = Foo.new()
foo.bar = 4294967296
""", stderr=test_stderr)

    def test_Struct240(self):
        def test_stderr(stderr):
            assert 0 < stderr.find("TypeError: Value must be Fixnum, not String")
        self._test("""
Foo = StructClass.new(\"Foo\", [[\'ulong, \'bar]])
foo = Foo.new()
foo.bar = \"baz\"
""", stderr=test_stderr)

    # Tests for slong
    def test_Struct250(self):
        def test_stderr(stderr):
            assert 0 < stderr.find("ValueError: Value exceeds range of slong")
        self._test("""
Foo = StructClass.new(\"Foo\", [[\'slong, \'bar]])
foo = Foo.new()
foo.bar = -2147483649
""", stderr=test_stderr)

    def test_Struct260(self):
        self._test("""
Foo = StructClass.new(\"Foo\", [[\'slong, \'bar]])
foo = Foo.new()
foo.bar = -2147483648
print(foo.bar)
""", "-2147483648")

    def test_Struct270(self):
        self._test("""
Foo = StructClass.new(\"Foo\", [[\'slong, \'bar]])
foo = Foo.new()
foo.bar = 2147483647
print(foo.bar)
""", "2147483647")

    def test_Struct280(self):
        def test_stderr(stderr):
            assert 0 < stderr.find("ValueError: Value exceeds range of slong")
        self._test("""
Foo = StructClass.new(\"Foo\", [[\'slong, \'bar]])
foo = Foo.new()
foo.bar = 2147483648
""", stderr=test_stderr)

    def test_Struct290(self):
        def test_stderr(stderr):
            assert 0 < stderr.find("TypeError: Value must be Fixnum, not String")
        self._test("""
Foo = StructClass.new(\"Foo\", [[\'slong, \'bar]])
foo = Foo.new()
foo.bar = \"baz\"
""", stderr=test_stderr)

    # Tests for pointer
    def test_Struct300(self):
        def test_stderr(stderr):
            assert 0 < stderr.find("ValueError: Value exceeds range of pointer")
        self._test("""
Foo = StructClass.new(\"Foo\", [[\'pointer, \'bar]])
foo = Foo.new()
foo.bar = -1
""", stderr=test_stderr)

    def test_Struct310(self):
        self._test("""
Foo = StructClass.new(\"Foo\", [[\'pointer, \'bar]])
foo = Foo.new()
foo.bar = 0
print(foo.bar)
""", "0")

    def test_Struct320(self):
        self._test("""
Foo = StructClass.new(\"Foo\", [[\'pointer, \'bar]])
foo = Foo.new()
foo.bar = 4294967295
print(foo.bar)
""", "4294967295")

    def test_Struct330(self):
        def test_stderr(stderr):
            assert 0 < stderr.find("ValueError: Value exceeds range of pointer")
        self._test("""
Foo = StructClass.new(\"Foo\", [[\'pointer, \'bar]])
foo = Foo.new()
foo.bar = 4294967296
""", stderr=test_stderr)

    def test_Struct340(self):
        def test_stderr(stderr):
            assert 0 < stderr.find("TypeError: Value must be Fixnum, not String")
        self._test("""
Foo = StructClass.new(\"Foo\", [[\'pointer, \'bar]])
foo = Foo.new()
foo.bar = \"baz\"
""", stderr=test_stderr)

# vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
