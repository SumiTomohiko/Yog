# -*- coding: utf-8 -*-

from testcase import TestCase

class TestFloat(TestCase):

    def test_float0(self):
        self._test("""
puts(3.1415926535)
""", """3.1415926535
""")

    def test_negative0(self):
        self._test("""
puts(- 3.1415926535)
""", """-3.1415926535
""")

    def test_positive0(self):
        self._test("""
puts(+ 3.1415926535)
""", """3.1415926535
""")

    def test_add0(self):
        self._test("""
# Float + Fixnum
puts(3.141592 + 42)
""", """45.141592
""")

    def test_add10(self):
        def test_stdout(stdout):
            self._test_regexp(r"4\.61168601843e\+0*18", stdout)

        self._test("""
# Float + Bignum
print(3.141592 + 4611686018427387904)
""", stdout=test_stdout)

    def test_add20(self):
        self._test("""
# Float + Float
puts(3.141592 + 2.71828183)
""", """5.85987383
""")

    def test_add30(self):
        def test_stderr(stderr):
            self._test_regexp(r"""Traceback \(most recent call last\):
  File "[^"]+", line 3, in <package>
TypeError: unsupported operand type\(s\) for \+: Float and Bool
""", stderr)

        self._test("""
# Float + Bool (TypeError)
puts(3.1415926535 + true)
""", stderr=test_stderr)

    def test_add40(self):
        def test_stderr(stderr):
            self._test_regexp(r"""Traceback \(most recent call last\):
  File "[^"]+", line 3, in <package>
TypeError: unsupported operand type\(s\) for \+: Float and Nil
""", stderr)

        self._test("""
# Float + nil (TypeError)
puts(3.1415926535 + nil)
""", stderr=test_stderr)

    def test_add50(self):
        def test_stderr(stderr):
            self._test_regexp(r"""Traceback \(most recent call last\):
  File "[^"]+", line 3, in <package>
TypeError: unsupported operand type\(s\) for \+: Float and Symbol
""", stderr)

        self._test("""
# Float + Symbol (TypeError)
puts(3.1415926535 + 'foo)
""", stderr=test_stderr)

    def test_subtract0(self):
        self._test("""
# Float - Fixnum
puts(3.141592 - 42)
""", """-38.858408
""")

    def test_subtract10(self):
        def test_stdout(stdout):
            self._test_regexp(r"-4\.61168601843e\+0*18", stdout)

        self._test("""
# Float - Bignum
print(3.141592 - 4611686018427387904)
""", stdout=test_stdout)

    def test_subtract20(self):
        self._test("""
# Float - Float
puts(3.141592 - 2.71828183)
""", """0.42331017
""")

    def test_subtract30(self):
        def test_stderr(stderr):
            self._test_regexp(r"""Traceback \(most recent call last\):
  File "[^"]+", line 3, in <package>
TypeError: unsupported operand type\(s\) for -: Float and Bool
""", stderr)

        self._test("""
# Float - Bool (TypeError)
puts(3.1415926535 - true)
""", stderr=test_stderr)

    def test_subtract40(self):
        def test_stderr(stderr):
            self._test_regexp(r"""Traceback \(most recent call last\):
  File "[^"]+", line 3, in <package>
TypeError: unsupported operand type\(s\) for -: Float and Nil
""", stderr)

        self._test("""
# Float - nil (TypeError)
puts(3.1415926535 - nil)
""", stderr=test_stderr)

    def test_subtract50(self):
        def test_stderr(stderr):
            self._test_regexp(r"""Traceback \(most recent call last\):
  File "[^"]+", line 3, in <package>
TypeError: unsupported operand type\(s\) for -: Float and Symbol
""", stderr)

        self._test("""
# Float - Symbol (TypeError)
puts(3.1415926535 - 'foo)
""", stderr=test_stderr)

    def test_multiply0(self):
        def test_stdout(stdout):
            self._test_regexp(r"131\.946864", stdout)

        self._test("""
# Float * Fixnum
print(3.141592 * 42)
""", stdout=test_stdout)

    def test_multiply10(self):
        def test_stdout(stdout):
            self._test_regexp(r"1\.4488035902e\+0*19", stdout)

        self._test("""
# Float * Bignum
print(3.141592 * 4611686018427387904)
""", stdout=test_stdout)

    def test_multiply20(self):
        self._test("""
# Float * Float
puts(3.141592 * 2.71828183)
""", """8.53973245087
""")

    def test_multiply30(self):
        def test_stderr(stderr):
            self._test_regexp(r"""Traceback \(most recent call last\):
  File "[^"]+", line 3, in <package>
TypeError: unsupported operand type\(s\) for \*: Float and Bool
""", stderr)

        self._test("""
# Float * Bool (TypeError)
puts(3.1415926535 * true)
""", stderr=test_stderr)

    def test_multiply40(self):
        def test_stderr(stderr):
            self._test_regexp(r"""Traceback \(most recent call last\):
  File "[^"]+", line 3, in <package>
TypeError: unsupported operand type\(s\) for \*: Float and Nil
""", stderr)

        self._test("""
# Float * nil (TypeError)
puts(3.1415926535 * nil)
""", stderr=test_stderr)

    def test_multiply50(self):
        def test_stderr(stderr):
            self._test_regexp(r"""Traceback \(most recent call last\):
  File "[^"]+", line 3, in <package>
TypeError: unsupported operand type\(s\) for \*: Float and Symbol
""", stderr)

        self._test("""
# Float * Symbol (TypeError)
puts(3.1415926535 * 'foo)
""", stderr=test_stderr)

    def test_divide0(self):
        self._test("""
# Float / Fixnum
puts(3.141592 / 42)
""", """0.0747998095238
""")

    def test_divide10(self):
        self._test("""
# Float / Bignum
puts(4611686018427387904.0 / 4611686018427387904)
""", """1.0
""")

    def test_divide20(self):
        self._test("""
# Float / Float
puts(3.141592 / 2.71828183)
""", """1.15572710869
""")

    def test_divide30(self):
        def test_stderr(stderr):
            self._test_regexp(r"""Traceback \(most recent call last\):
  File "[^"]+", line 3, in <package>
TypeError: unsupported operand type\(s\) for /: Float and Bool
""", stderr)

        self._test("""
# Float / Bool (TypeError)
puts(3.1415926535 / true)
""", stderr=test_stderr)

    def test_divide40(self):
        def test_stderr(stderr):
            self._test_regexp(r"""Traceback \(most recent call last\):
  File "[^"]+", line 3, in <package>
TypeError: unsupported operand type\(s\) for /: Float and Nil
""", stderr)

        self._test("""
# Float / nil (TypeError)
puts(3.1415926535 / nil)
""", stderr=test_stderr)

    def test_divide50(self):
        def test_stderr(stderr):
            self._test_regexp(r"""Traceback \(most recent call last\):
  File "[^"]+", line 3, in <package>
TypeError: unsupported operand type\(s\) for /: Float and Symbol
""", stderr)

        self._test("""
# Float / Symbol (TypeError)
puts(3.1415926535 / 'foo)
""", stderr=test_stderr)

    def test_floor_divide0(self):
        self._test("""
# Float // Fixnum
puts(3.141592 // 42)
""", """0.0747998095238
""")

    def test_floor_divide10(self):
        self._test("""
# Float // Bignum
puts(4611686018427387904.0 // 4611686018427387904)
""", """1.0
""")

    def test_floor_divide20(self):
        self._test("""
# Float // Float
puts(3.141592 // 2.71828183)
""", """1.15572710869
""")

    def test_floor_divide30(self):
        def test_stderr(stderr):
            self._test_regexp(r"""Traceback \(most recent call last\):
  File "[^"]+", line 3, in <package>
TypeError: unsupported operand type\(s\) for //: Float and Bool
""", stderr)

        self._test("""
# Float // Bool (TypeError)
puts(3.1415926535 // true)
""", stderr=test_stderr)

    def test_floor_divide40(self):
        def test_stderr(stderr):
            self._test_regexp(r"""Traceback \(most recent call last\):
  File "[^"]+", line 3, in <package>
TypeError: unsupported operand type\(s\) for //: Float and Nil
""", stderr)

        self._test("""
# Float // nil (TypeError)
puts(3.1415926535 // nil)
""", stderr=test_stderr)

    def test_floor_divide50(self):
        def test_stderr(stderr):
            self._test_regexp(r"""Traceback \(most recent call last\):
  File "[^"]+", line 3, in <package>
TypeError: unsupported operand type\(s\) for //: Float and Symbol
""", stderr)

        self._test("""
# Float // Symbol (TypeError)
puts(3.1415926535 // 'foo)
""", stderr=test_stderr)

    def test_power0(self):
        def test_stdout(stdout):
            self._test_regexp(r"7\.59092416294e\+0*20", stdout)

        self._test("""
# Float ** Fixnum
print(3.1415926535 ** 42)
""", stdout=test_stdout)

    def test_power10(self):
        def test_stderr(stderr):
            self._test_regexp(r"""Traceback \(most recent call last\):
  File "[^"]+", line 3, in <package>
ZeroDivisionError: 0.0 cannot be raised to a negative power
""", stderr)

        self._test("""
# Float ** Fixnum
print(0.0 ** (- 42))
""", stderr=test_stderr)

    def test_power20(self):
        self._test("""
print(3.1415926535 ** 2.71828183)
""", "22.4591577562")

    def test_power30(self):
        def test_stderr(stderr):
            self._test_regexp(r"""Traceback \(most recent call last\):
  File "[^"]+", line 3, in <package>
ZeroDivisionError: 0.0 cannot be raised to a negative power
""", stderr)

        self._test("""
# Float ** Float
print(0.0 ** (- 3.1415926535))
""", stderr=test_stderr)

    def test_hash0(self):
        self._test("""
f = 3.1415926535
d = {}
d[f] = 42
print(d[f])
""", "42")

    def test_compare0(self):
        self._test("""
print(2.71828183 < 3.1415926535)
""", "true")

    def test_compare10(self):
        self._test("""
print(3.1415926535 < 2.71828183)
""", "false")

    def test_compare20(self):
        self._test("""
print(3.1415926535 == 3.1415926535)
""", "true")

    def test_compare30(self):
        self._test("""
print(2.71828183 == 3.1415926535)
""", "false")

    def test_compare40(self):
        self._test("""
print(3.1415926535 != 3.1415926535)
""", "false")

    def test_compare50(self):
        self._test("""
print(2.71828183 != 3.1415926535)
""", "true")

# vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
