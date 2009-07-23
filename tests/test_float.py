# -*- coding: utf-8 -*-

from re import match
from testcase import TestCase

class TestFloat(TestCase):

    def test_float0(self):
        self._test("""
puts(3.1415926535)
""", """3.14159
""")

    def test_negative0(self):
        self._test("""
puts(- 3.1415926535)
""", """-3.14159
""")

    def test_add0(self):
        self._test("""
# Float + int
puts(3.141592 + 42)
""", """45.1416
""")

    def test_add10(self):
        self._test("""
# Float + Bignum
puts(3.141592 + 4611686018427387904)
""", """4.61169e+18
""")

    def test_add20(self):
        self._test("""
# Float + Float
puts(3.141592 + 2.71828183)
""", """5.85987
""")

    def test_add30(self):
        def test_stderr(stderr):
            m = match(r"""Traceback \(most recent call last\):
  File "[^"]+", line 3, in <module>
  File builtin, in Float#\+
TypeError: unsupported operand type\(s\) for \+: 'Float' and 'Bool'
""", stderr)
            assert m is not None

        self._test("""
# Float + Bool (TypeError)
puts(3.1415926535 + true)
""", stderr=test_stderr)

    def test_add40(self):
        def test_stderr(stderr):
            m = match(r"""Traceback \(most recent call last\):
  File "[^"]+", line 3, in <module>
  File builtin, in Float#\+
TypeError: unsupported operand type\(s\) for \+: 'Float' and 'Nil'
""", stderr)
            assert m is not None

        self._test("""
# Float + nil (TypeError)
puts(3.1415926535 + nil)
""", stderr=test_stderr)

    def test_add50(self):
        def test_stderr(stderr):
            m = match(r"""Traceback \(most recent call last\):
  File "[^"]+", line 3, in <module>
  File builtin, in Float#\+
TypeError: unsupported operand type\(s\) for \+: 'Float' and 'Symbol'
""", stderr)
            assert m is not None

        self._test("""
# Float + Symbol (TypeError)
puts(3.1415926535 + :foo)
""", stderr=test_stderr)

    def test_subtract0(self):
        self._test("""
# Float - int
puts(3.141592 - 42)
""", """-38.8584
""")

    def test_subtract10(self):
        self._test("""
# Float - Bignum
puts(3.141592 - 4611686018427387904)
""", """-4.61169e+18
""")

    def test_subtract20(self):
        self._test("""
# Float - Float
puts(3.141592 - 2.71828183)
""", """0.42331
""")

    def test_subtract30(self):
        def test_stderr(stderr):
            m = match(r"""Traceback \(most recent call last\):
  File "[^"]+", line 3, in <module>
  File builtin, in Float#-
TypeError: unsupported operand type\(s\) for -: 'Float' and 'Bool'
""", stderr)
            assert m is not None

        self._test("""
# Float - Bool (TypeError)
puts(3.1415926535 - true)
""", stderr=test_stderr)

    def test_subtract40(self):
        def test_stderr(stderr):
            m = match(r"""Traceback \(most recent call last\):
  File "[^"]+", line 3, in <module>
  File builtin, in Float#-
TypeError: unsupported operand type\(s\) for -: 'Float' and 'Nil'
""", stderr)
            assert m is not None

        self._test("""
# Float - nil (TypeError)
puts(3.1415926535 - nil)
""", stderr=test_stderr)

    def test_subtract50(self):
        def test_stderr(stderr):
            m = match(r"""Traceback \(most recent call last\):
  File "[^"]+", line 3, in <module>
  File builtin, in Float#-
TypeError: unsupported operand type\(s\) for -: 'Float' and 'Symbol'
""", stderr)
            assert m is not None

        self._test("""
# Float - Symbol (TypeError)
puts(3.1415926535 - :foo)
""", stderr=test_stderr)

    def test_multiply0(self):
        self._test("""
# Float * int
puts(3.141592 * 42)
""", """131.947
""")

    def test_multiply10(self):
        self._test("""
# Float * Bignum
puts(3.141592 * 4611686018427387904)
""", """1.4488e+19
""")

    def test_multiply20(self):
        self._test("""
# Float * Float
puts(3.141592 * 2.71828183)
""", """8.53973
""")

    def test_multiply30(self):
        def test_stderr(stderr):
            m = match(r"""Traceback \(most recent call last\):
  File "[^"]+", line 3, in <module>
  File builtin, in Float#\*
TypeError: unsupported operand type\(s\) for \*: 'Float' and 'Bool'
""", stderr)
            assert m is not None

        self._test("""
# Float * Bool (TypeError)
puts(3.1415926535 * true)
""", stderr=test_stderr)

    def test_multiply40(self):
        def test_stderr(stderr):
            m = match(r"""Traceback \(most recent call last\):
  File "[^"]+", line 3, in <module>
  File builtin, in Float#\*
TypeError: unsupported operand type\(s\) for \*: 'Float' and 'Nil'
""", stderr)
            assert m is not None

        self._test("""
# Float * nil (TypeError)
puts(3.1415926535 * nil)
""", stderr=test_stderr)

    def test_multiply50(self):
        def test_stderr(stderr):
            m = match(r"""Traceback \(most recent call last\):
  File "[^"]+", line 3, in <module>
  File builtin, in Float#\*
TypeError: unsupported operand type\(s\) for \*: 'Float' and 'Symbol'
""", stderr)
            assert m is not None

        self._test("""
# Float * Symbol (TypeError)
puts(3.1415926535 * :foo)
""", stderr=test_stderr)

    def test_divide0(self):
        self._test("""
# Float / int
puts(3.141592 / 42)
""", """0.0747998
""")

    def test_divide10(self):
        self._test("""
# Float / Bignum
puts(4611686018427387904.0 / 4611686018427387904)
""", """1
""")

    def test_divide20(self):
        self._test("""
# Float / Float
puts(3.141592 / 2.71828183)
""", """1.15573
""")

    def test_divide30(self):
        def test_stderr(stderr):
            m = match(r"""Traceback \(most recent call last\):
  File "[^"]+", line 3, in <module>
  File builtin, in Float#/
TypeError: unsupported operand type\(s\) for /: 'Float' and 'Bool'
""", stderr)
            assert m is not None

        self._test("""
# Float / Bool (TypeError)
puts(3.1415926535 / true)
""", stderr=test_stderr)

    def test_divide40(self):
        def test_stderr(stderr):
            m = match(r"""Traceback \(most recent call last\):
  File "[^"]+", line 3, in <module>
  File builtin, in Float#/
TypeError: unsupported operand type\(s\) for /: 'Float' and 'Nil'
""", stderr)
            assert m is not None

        self._test("""
# Float / nil (TypeError)
puts(3.1415926535 / nil)
""", stderr=test_stderr)

    def test_divide50(self):
        def test_stderr(stderr):
            m = match(r"""Traceback \(most recent call last\):
  File "[^"]+", line 3, in <module>
  File builtin, in Float#/
TypeError: unsupported operand type\(s\) for /: 'Float' and 'Symbol'
""", stderr)
            assert m is not None

        self._test("""
# Float / Symbol (TypeError)
puts(3.1415926535 / :foo)
""", stderr=test_stderr)

# vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
