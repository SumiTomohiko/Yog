
from testcase import get_lib_path
from unix import TestUnix

class TestFFI(TestUnix):

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

    # Tests for uint64
    def test_argument360(self):
        def test_stderr(stderr):
            assert 0 < stderr.find("ValueError: Value must be greater or equal 0, not -1")
        path = get_lib_path()
        self._test("""
lib = load_lib(\"%(path)s\")
f = lib.load_func(\"print_uint64\", [\'uint64])
f(-1)
""" % locals(), stderr=test_stderr)

    def test_argument370(self):
        path = get_lib_path()
        self._test("""
lib = load_lib(\"%(path)s\")
f = lib.load_func(\"print_uint64\", [\'uint64])
f(0)
""" % locals(), "0")

    def test_argument380(self):
        path = get_lib_path()
        self._test("""
lib = load_lib(\"%(path)s\")
f = lib.load_func(\"print_uint64\", [\'uint64])
f(18446744073709551615)
""" % locals(), "18446744073709551615")

    def test_argument390(self):
        def test_stderr(stderr):
            assert 0 < stderr.find("ValueError: Value must be less or equal 18446744073709551615, not 18446744073709551616")
        path = get_lib_path()
        self._test("""
lib = load_lib(\"%(path)s\")
f = lib.load_func(\"print_uint64\", [\'uint64])
f(18446744073709551616)
""" % locals(), stderr=test_stderr)

    def test_argument400(self):
        def test_stderr(stderr):
            assert 0 < stderr.find("TypeError: Value must be Fixnum or Bignum, not String")
        path = get_lib_path()
        self._test("""
lib = load_lib(\"%(path)s\")
f = lib.load_func(\"print_uint64\", [\'uint64])
f(\"baz\")
""" % locals(), stderr=test_stderr)

    # Tests for int64
    def test_argument410(self):
        def test_stderr(stderr):
            assert 0 < stderr.find("ValueError: Value must be greater or equal -9223372036854775808, not -9223372036854775809")
        path = get_lib_path()
        self._test("""
lib = load_lib(\"%(path)s\")
f = lib.load_func(\"print_int64\", [\'int64])
f(-9223372036854775809)
""" % locals(), stderr=test_stderr)

    def test_argument420(self):
        path = get_lib_path()
        self._test("""
lib = load_lib(\"%(path)s\")
f = lib.load_func(\"print_int64\", [\'int64])
f(-9223372036854775808)
""" % locals(), "-9223372036854775808")

    def test_argument430(self):
        path = get_lib_path()
        self._test("""
lib = load_lib(\"%(path)s\")
f = lib.load_func(\"print_int64\", [\'int64])
f(9223372036854775807)
""" % locals(), "9223372036854775807")

    def test_argument440(self):
        def test_stderr(stderr):
            assert 0 < stderr.find("ValueError: Value must be less or equal 9223372036854775807, not 9223372036854775808")
        path = get_lib_path()
        self._test("""
lib = load_lib(\"%(path)s\")
f = lib.load_func(\"print_int64\", [\'int64])
f(9223372036854775808)
""" % locals(), stderr=test_stderr)

    def test_argument450(self):
        def test_stderr(stderr):
            assert 0 < stderr.find("TypeError: Value must be Fixnum or Bignum, not String")
        path = get_lib_path()
        self._test("""
lib = load_lib(\"%(path)s\")
f = lib.load_func(\"print_int64\", [\'int64])
f(\"baz\")
""" % locals(), stderr=test_stderr)

    def test_return100(self):
        path = get_lib_path()
        self._test("""
lib = load_lib(\"%(path)s\")
f = lib.load_func(\"return_uint64_0\", [], \'uint64)
print(f())
""" % locals(), "1073741823")

    def test_return110(self):
        path = get_lib_path()
        self._test("""
lib = load_lib(\"%(path)s\")
f = lib.load_func(\"return_uint64_1\", [], \'uint64)
print(f())
""" % locals(), "1073741824")

    def test_return120(self):
        path = get_lib_path()
        self._test("""
lib = load_lib(\"%(path)s\")
f = lib.load_func(\"return_int64_0\", [], \'int64)
print(f())
""" % locals(), "-4611686018427387905")

    def test_return130(self):
        path = get_lib_path()
        self._test("""
lib = load_lib(\"%(path)s\")
f = lib.load_func(\"return_int64_1\", [], \'int64)
print(f())
""" % locals(), "-4611686018427387904")

    def test_return140(self):
        path = get_lib_path()
        self._test("""
lib = load_lib(\"%(path)s\")
f = lib.load_func(\"return_int64_2\", [], \'int64)
print(f())
""" % locals(), "-1073741825")

    def test_return150(self):
        path = get_lib_path()
        self._test("""
lib = load_lib(\"%(path)s\")
f = lib.load_func(\"return_int64_3\", [], \'int64)
print(f())
""" % locals(), "-1073741824")

    def test_return160(self):
        path = get_lib_path()
        self._test("""
lib = load_lib(\"%(path)s\")
f = lib.load_func(\"return_int64_4\", [], \'int64)
print(f())
""" % locals(), "1073741823")

    def test_return170(self):
        path = get_lib_path()
        self._test("""
lib = load_lib(\"%(path)s\")
f = lib.load_func(\"return_int64_5\", [], \'int64)
print(f())
""" % locals(), "1073741824")

    def test_return180(self):
        path = get_lib_path()
        self._test("""
lib = load_lib(\"%(path)s\")
f = lib.load_func(\"return_int64_6\", [], \'int64)
print(f())
""" % locals(), "4611686018427387903")

    def test_return190(self):
        path = get_lib_path()
        self._test("""
lib = load_lib(\"%(path)s\")
f = lib.load_func(\"return_int64_7\", [], \'int64)
print(f())
""" % locals(), "4611686018427387904")

    # Tests for longdouble
    def test_argument600(self):
        path = get_lib_path()
        self._test("""
lib = load_lib(\"%(path)s\")
f = lib.load_func(\"print_longdouble\", [\'longdouble])
f(3.14)
""" % locals(), "3.14")

    def test_argument610(self):
        def test_stderr(stderr):
            assert 0 < stderr.find("TypeError: Value must be Float, not String")
        path = get_lib_path()
        self._test("""
lib = load_lib(\"%(path)s\")
f = lib.load_func(\"print_longdouble\", [\'longdouble])
f(\"baz\")
""" % locals(), stderr=test_stderr)

# vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
