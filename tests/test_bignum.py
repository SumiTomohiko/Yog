# -*- coding: utf-8 -*-

from re import match
from testcase import TestCase

class TestBignum(TestCase):

    def test_negative0(self):
        self._test("""
puts(- 4611686018427387905)
""", """-4611686018427387905
""")

    def test_add0(self):
        self._test("""
# Bignum + int
puts(4611686018427387904 + 1)
""", """4611686018427387905
""")

    def test_add10(self):
        self._test("""
# Bignum + Bignum
puts(4611686018427387904 + 4611686018427387904)
""", """9223372036854775808
""")

    def test_add20(self):
        self._test("""
# Bignum + Float
puts(4611686018427387904 + 3.1415926535)
""", """4.61169e+18
""")

    def test_add30(self):
        def test_stderr(stderr):
            m = match(r"""Traceback \(most recent call last\):
  File "[^"]+", line 3, in <module>
  File builtin, in Bignum#\+
TypeError: unsupported operand type\(s\) for \+: 'Bignum' and 'Bool'
""", stderr)
            assert m is not None

        self._test("""
# Bignum + Bool (TypeError)
puts(4611686018427387904 + true)
""", stderr=test_stderr)

    def test_add40(self):
        def test_stderr(stderr):
            m = match(r"""Traceback \(most recent call last\):
  File "[^"]+", line 3, in <module>
  File builtin, in Bignum#\+
TypeError: unsupported operand type\(s\) for \+: 'Bignum' and 'Nil'
""", stderr)
            assert m is not None

        self._test("""
# Bignum + nil (TypeError)
puts(4611686018427387904 + nil)
""", stderr=test_stderr)

    def test_add50(self):
        def test_stderr(stderr):
            m = match(r"""Traceback \(most recent call last\):
  File "[^"]+", line 3, in <module>
  File builtin, in Bignum#\+
TypeError: unsupported operand type\(s\) for \+: 'Bignum' and 'Symbol'
""", stderr)
            assert m is not None

        self._test("""
# Bignum + Symbol (TypeError)
puts(4611686018427387904 + :foo)
""", stderr=test_stderr)

    def test_subtract0(self):
        self._test("""
# Bignum - int = Bignum
puts(4611686018427387905 - 1)
""", """4611686018427387904
""")

    def test_subtract10(self):
        self._test("""
# Bignum - int = int (64bit) 
puts(4611686018427387904 - 1)
""", """4611686018427387903
""")

    def test_subtract20(self):
        self._test("""
# Bignum - int = int (32bit) 
puts(1073741824 - 1)
""", """1073741823
""")

    def test_subtract30(self):
        self._test("""
# Bignum - Bignum = int
puts(4611686018427387905 - 4611686018427387904)
""", """1
""")

    def test_subtract40(self):
        self._test("""
# Bignum - Bignum = Bignum
puts(13835058055282163712 - 9223372036854775808)
""", """4611686018427387904
""")

    def test_subtract50(self):
        self._test("""
# Bignum - Float
puts(4611686018427387904 - 3.1415926535)
""", """4.61169e+18
""")

    def test_subtract60(self):
        def test_stderr(stderr):
            m = match(r"""Traceback \(most recent call last\):
  File "[^"]+", line 3, in <module>
  File builtin, in Bignum#-
TypeError: unsupported operand type\(s\) for -: 'Bignum' and 'Bool'
""", stderr)
            assert m is not None

        self._test("""
# Bignum - Bool (TypeError)
puts(4611686018427387904 - true)
""", stderr=test_stderr)

    def test_subtract70(self):
        def test_stderr(stderr):
            m = match(r"""Traceback \(most recent call last\):
  File "[^"]+", line 3, in <module>
  File builtin, in Bignum#-
TypeError: unsupported operand type\(s\) for -: 'Bignum' and 'Nil'
""", stderr)
            assert m is not None

        self._test("""
# Bignum - nil (TypeError)
puts(4611686018427387904 - nil)
""", stderr=test_stderr)

    def test_subtract80(self):
        def test_stderr(stderr):
            m = match(r"""Traceback \(most recent call last\):
  File "[^"]+", line 3, in <module>
  File builtin, in Bignum#-
TypeError: unsupported operand type\(s\) for -: 'Bignum' and 'Symbol'
""", stderr)
            assert m is not None

        self._test("""
# Bignum - Symbol (TypeError)
puts(4611686018427387904 - :foo)
""", stderr=test_stderr)

    def test_multiply0(self):
        self._test("""
# Bignum * int
puts(4611686018427387904 * 42)
""", """193690812773950291968
""")

    def test_multiply10(self):
        self._test("""
# Bignum * Bignum
puts(4611686018427387904 * 4611686018427387904)
""", """21267647932558653966460912964485513216
""")

    def test_multiply20(self):
        self._test("""
# Bignum * Float
puts(4611686018427387904 * 3.1415926535)
""", """1.4488e+19
""")

    def test_multiply30(self):
        def test_stderr(stderr):
            m = match(r"""Traceback \(most recent call last\):
  File "[^"]+", line 3, in <module>
  File builtin, in Bignum#\*
TypeError: unsupported operand type\(s\) for \*: 'Bignum' and 'Bool'
""", stderr)
            assert m is not None

        self._test("""
# Bignum * Bool (TypeError)
puts(4611686018427387904 * true)
""", stderr=test_stderr)

    def test_multiply40(self):
        def test_stderr(stderr):
            m = match(r"""Traceback \(most recent call last\):
  File "[^"]+", line 3, in <module>
  File builtin, in Bignum#\*
TypeError: unsupported operand type\(s\) for \*: 'Bignum' and 'Nil'
""", stderr)
            assert m is not None

        self._test("""
# Bignum * nil (TypeError)
puts(4611686018427387904 * nil)
""", stderr=test_stderr)

    def test_multiply50(self):
        def test_stderr(stderr):
            m = match(r"""Traceback \(most recent call last\):
  File "[^"]+", line 3, in <module>
  File builtin, in Bignum#\*
TypeError: unsupported operand type\(s\) for \*: 'Bignum' and 'Symbol'
""", stderr)
            assert m is not None

        self._test("""
# Bignum * Symbol (TypeError)
puts(4611686018427387904 * :foo)
""", stderr=test_stderr)

    def test_divide0(self):
        self._test("""
# Bignum / int
puts(4611686018427387904 / 42)
""", """1.09802e+17
""")

    def test_divide10(self):
        self._test("""
# Bignum / Float
puts(4611686018427387904 / 3.1415926535)
""", """1.46795e+18
""")

    def test_divide20(self):
        self._test("""
# Bignum / Bignum
puts(4611686018427387904 / 9223372036854775808)
""", """0.5
""")

    def test_divide40(self):
        def test_stderr(stderr):
            m = match(r"""Traceback \(most recent call last\):
  File "[^"]+", line 3, in <module>
  File builtin, in Bignum#/
TypeError: unsupported operand type\(s\) for /: 'Bignum' and 'Bool'
""", stderr)
            assert m is not None

        self._test("""
# Bignum / Bool (TypeError)
puts(4611686018427387904 / true)
""", stderr=test_stderr)

    def test_divide50(self):
        def test_stderr(stderr):
            m = match(r"""Traceback \(most recent call last\):
  File "[^"]+", line 3, in <module>
  File builtin, in Bignum#/
TypeError: unsupported operand type\(s\) for /: 'Bignum' and 'Nil'
""", stderr)
            assert m is not None

        self._test("""
# Bignum / nil (TypeError)
puts(4611686018427387904 / nil)
""", stderr=test_stderr)

    def test_divide60(self):
        def test_stderr(stderr):
            m = match(r"""Traceback \(most recent call last\):
  File "[^"]+", line 3, in <module>
  File builtin, in Bignum#/
TypeError: unsupported operand type\(s\) for /: 'Bignum' and 'Symbol'
""", stderr)
            assert m is not None

        self._test("""
# Bignum / Symbol (TypeError)
puts(4611686018427387904 / :foo)
""", stderr=test_stderr)

    def test_divide70(self):
        def test_stderr(stderr):
            m = match(r"""Traceback \(most recent call last\):
  File "[^"]+", line 3, in <module>
  File builtin, in Bignum#/
ZeroDivisionError: Bignum division by zero
""", stderr)
            assert m is not None

        self._test("""
# Bignum / zero (ZeroDivisionError)
puts(4611686018427387904 / 0)
""", stderr=test_stderr)

    def test_divide80(self):
        def test_stderr(stderr):
            m = match(r"""Traceback \(most recent call last\):
  File "[^"]+", line 3, in <module>
  File builtin, in Bignum#/
ZeroDivisionError: float division
""", stderr)
            assert m is not None

        self._test("""
# Bignum / 0.0 (ZeroDivisionError)
puts(4611686018427387904 / 0.0)
""", stderr=test_stderr)

    def test_floor_divide0(self):
        self._test("""
# Bignum // int = int (32bit)
puts(1073741824 // 1073741823)
""", """1
""")

    def test_floor_divide10(self):
        self._test("""
# Bignum // int = int (64bit)
puts(4611686018427387904 // 4611686018427387903)
""", """1
""")

    def test_floor_divide20(self):
        self._test("""
# Bignum // Float = Float
puts(4611686018427387904 // 3.1415926535)
""", """1.46795e+18
""")

    def test_floor_divide30(self):
        self._test("""
# Bignum // Bignum = int
puts(9223372036854775808 // 4611686018427387904)
""", """2
""")

    def test_floor_divide40(self):
        self._test("""
# Bignum // Bignum = Bignum
puts(21267647932558653966460912964485513216 // 4611686018427387904)
""", """4611686018427387904
""")

    def test_floor_divide50(self):
        def test_stderr(stderr):
            m = match(r"""Traceback \(most recent call last\):
  File "[^"]+", line 3, in <module>
  File builtin, in Bignum#//
TypeError: unsupported operand type\(s\) for //: 'Bignum' and 'Bool'
""", stderr)
            assert m is not None

        self._test("""
# Bignum // Bool (TypeError)
puts(4611686018427387904 // true)
""", stderr=test_stderr)

    def test_floor_divide60(self):
        def test_stderr(stderr):
            m = match(r"""Traceback \(most recent call last\):
  File "[^"]+", line 3, in <module>
  File builtin, in Bignum#//
TypeError: unsupported operand type\(s\) for //: 'Bignum' and 'Nil'
""", stderr)
            assert m is not None

        self._test("""
# Bignum // nil (TypeError)
puts(4611686018427387904 // nil)
""", stderr=test_stderr)

    def test_floor_divide70(self):
        def test_stderr(stderr):
            m = match(r"""Traceback \(most recent call last\):
  File "[^"]+", line 3, in <module>
  File builtin, in Bignum#//
TypeError: unsupported operand type\(s\) for //: 'Bignum' and 'Symbol'
""", stderr)
            assert m is not None

        self._test("""
# Bignum // Symbol (TypeError)
puts(4611686018427387904 // :foo)
""", stderr=test_stderr)

    def test_floor_divide80(self):
        def test_stderr(stderr):
            m = match(r"""Traceback \(most recent call last\):
  File "[^"]+", line 3, in <module>
  File builtin, in Bignum#//
ZeroDivisionError: Bignum division by zero
""", stderr)
            assert m is not None

        self._test("""
# Bignum // zero (ZeroDivisionError)
puts(4611686018427387904 // 0)
""", stderr=test_stderr)

    def test_floor_divide90(self):
        def test_stderr(stderr):
            m = match(r"""Traceback \(most recent call last\):
  File "[^"]+", line 3, in <module>
  File builtin, in Bignum#//
ZeroDivisionError: float division
""", stderr)
            assert m is not None

        self._test("""
# Bignum // 0.0 (ZeroDivisionError)
puts(4611686018427387904 // 0.0)
""", stderr=test_stderr)

# vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
