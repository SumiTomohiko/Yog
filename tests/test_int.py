# -*- coding: utf-8 -*-

from re import match
from testcase import TestCase

class TestInt(TestCase):

    def test_plus_one(self):
        self._test("""
puts(42 + 1)
""", "43\n")

    def test_compare1(self):
        self._test("""
puts(0 < 42)
""", "true\n")

    def test_compare2(self):
        self._test("""
puts(42 < 0)
""", "false\n")

    def test_to_s(self):
        self._test("""
puts(42.to_s())
""", """42
""")

    def test_times(self):
        self._test("""
10.times() do [n]
    puts(n)
end
""", """0
1
2
3
4
5
6
7
8
9
""")

    def test_bignum_literal1(self):
        self._test("""
puts(1073741824)
""", """1073741824
""")

    def test_bignum_literal2(self):
        self._test("""
puts(4611686018427387904)
""", """4611686018427387904
""")

    def test_binary_literal0(self):
        self._test("""
puts(0b101010)
""", """42
""")

    def test_binary_literal10(self):
        self._test("""
puts(0B101010)
""", """42
""")

    def test_binary_literal20(self):
        self._test("""
puts(0b0_0_1_0_1_0_1_0)
""", """42
""")

    def test_binary_literal30(self):
        self._test("""
puts(0b0010__1010)
""", stderr="""puts(0b0010__1010)
            ^
SyntaxError: trailing `_' in number
""")

    def test_binary_literal40(self):
        self._test("""
puts(0b_)
""", stderr="""puts(0b_)
       ^
SyntaxError: numeric literal without digits
""")

    def test_binary_literal50(self):
        self._test("""
puts(0b2)
""", stderr="""puts(0b2)
       ^
SyntaxError: numeric literal without digits
""")

    def test_binary_literal60(self):
        self._test("""
puts(0b1_2)
""", stderr="""puts(0b1_2)
         ^
SyntaxError: numeric literal without digits
""")

    def test_add0(self):
        self._test("""
puts(42 + 26)
""", """68
""")

    def test_add10(self):
        self._test("""
# int + int = Bignum (32bit)
puts(1 + 1073741823)
""", """1073741824
""")

    def test_add20(self):
        self._test("""
# int + int = Bignum (64bit)
puts(1 + 4611686018427387903)
""", """4611686018427387904
""")

    def test_add30(self):
        self._test("""
# int + Bignum (32bit)
puts(1 + 1073741824)
""", """1073741825
""")

    def test_add40(self):
        self._test("""
# int + Bignum (64bit)
puts(1 + 4611686018427387904)
""", """4611686018427387905
""")

    def test_negative0(self):
        self._test("""
puts(- 42)
""", """-42
""")

    def test_subtract0(self):
        self._test("""
# int - int = int
puts(42 - 26)
""", """16
""")

    def test_subtract10(self):
        self._test("""
# int - int = Bignum (32bit)
puts(- 1 - 1073741823)
""", """-1073741824
""")

    def test_subtract20(self):
        self._test("""
# int - int = Bignum (64bit)
puts(- 1 - 4611686018427387903)
""", """-4611686018427387904
""")

    def test_subtract30(self):
        self._test("""
# int - Bignum (32bit)
puts(- 1 - 1073741824)
""", """-1073741825
""")

    def test_subtract40(self):
        self._test("""
# int - Bignum (64bit)
puts(- 1 - 4611686018427387904)
""", """-4611686018427387905
""")

    def test_subtract50(self):
        self._test("""
# int - float
puts(- 42 - 3.141592)
""", """-45.1416
""")

    def test_multiply0(self):
        self._test("""
# int * int = int
puts(26 * 42)
""", """1092
""")

    def test_multiply10(self):
        self._test("""
# int * int = Bignum (32bit)
puts(2 * 536870912)
""", """1073741824
""")

    def test_multiply20(self):
        self._test("""
# int * int = Bignum (64bit)
puts(2 * 2305843009213693952)
""", """4611686018427387904
""")

    def test_multiply30(self):
        self._test("""
# int * Bignum = Bignum
puts(2 * 4611686018427387904)
""", """9223372036854775808
""")

    def test_multiply40(self):
        self._test("""
# int * float = float
puts(2 * 3.1415926535)
""", """6.28319
""")

    def test_multiply50(self):
        def test_stderr(stderr):
            m = match(r"""Traceback \(most recent call last\):
  File "[^"]+", line 3, in <module>
  File builtin, in Int#\*
TypeError: unsupported operand type\(s\) for \*: 'Int' and 'Bool'
""", stderr)
            assert m is not None

        self._test("""
# int * bool (TypeError)
puts(42 * true)
""", stderr=test_stderr)

    def test_multiply60(self):
        def test_stderr(stderr):
            m = match(r"""Traceback \(most recent call last\):
  File "[^"]+", line 3, in <module>
  File builtin, in Int#\*
TypeError: unsupported operand type\(s\) for \*: 'Int' and 'Nil'
""", stderr)
            assert m is not None

        self._test("""
# int * nil (TypeError)
puts(42 * nil)
""", stderr=test_stderr)

    def test_divide0(self):
        self._test("""
# int / int
puts(42 / 26)
""", """1.61538
""")

    def test_divide10(self):
        self._test("""
# int / float
puts(42 / 3.1415926535)
""", """13.369
""")

    def test_divide20(self):
        self._test("""
# int / Bignum (32bit)
puts(536870912 / 1073741824)
""", """0.5
""")

    def test_divide30(self):
        # TODO: enable this test
        return

        self._test("""
# int / Bignum (64bit)
puts(2305843009213693952 / 4611686018427387904)
""", """0.5
""")

    def test_divide40(self):
        def test_stderr(stderr):
            m = match(r"""Traceback \(most recent call last\):
  File "[^"]+", line 3, in <module>
  File builtin, in Int#/
TypeError: unsupported operand type\(s\) for /: 'Int' and 'Bool'
""", stderr)
            assert m is not None

        self._test("""
# int / bool (TypeError)
puts(42 / true)
""", stderr=test_stderr)

    def test_divide50(self):
        def test_stderr(stderr):
            m = match(r"""Traceback \(most recent call last\):
  File "[^"]+", line 3, in <module>
  File builtin, in Int#/
TypeError: unsupported operand type\(s\) for /: 'Int' and 'Nil'
""", stderr)
            assert m is not None

        self._test("""
# int / nil (TypeError)
puts(42 / nil)
""", stderr=test_stderr)

    def test_divide60(self):
        def test_stderr(stderr):
            m = match(r"""Traceback \(most recent call last\):
  File "[^"]+", line 3, in <module>
  File builtin, in Int#/
ZeroDivisionError: int division by zero
""", stderr)
            assert m is not None

        self._test("""
# int / zero (ZeroDivisionError)
puts(42 / 0)
""", stderr=test_stderr)

    def test_divide70(self):
        def test_stderr(stderr):
            m = match(r"""Traceback \(most recent call last\):
  File "[^"]+", line 3, in <module>
  File builtin, in Int#/
ZeroDivisionError: float division
""", stderr)
            assert m is not None

        self._test("""
# int / 0.0 (ZeroDivisionError)
puts(42 / 0.0)
""", stderr=test_stderr)

    def test_floor_divide0(self):
        self._test("""
# int // int
puts(42 // 26)
""", """1
""")

    def test_floor_divide10(self):
        self._test("""
# int // float
puts(42 // 3.1415926535)
""", """13.369
""")

    def test_floor_divide20(self):
        self._test("""
# int // Bignum (32bit)
puts(536870912 // 1073741824)
""", """0
""")

    def test_floor_divide30(self):
        # TODO: enable this test
        return

        self._test("""
# int // Bignum (64bit)
puts(2305843009213693952 // 4611686018427387904)
""", """0
""")

    def test_floor_divide40(self):
        def test_stderr(stderr):
            m = match(r"""Traceback \(most recent call last\):
  File "[^"]+", line 3, in <module>
  File builtin, in Int#//
TypeError: unsupported operand type\(s\) for //: 'Int' and 'Bool'
""", stderr)
            assert m is not None

        self._test("""
# int // bool (TypeError)
puts(42 // true)
""", stderr=test_stderr)

    def test_floor_divide50(self):
        def test_stderr(stderr):
            m = match(r"""Traceback \(most recent call last\):
  File "[^"]+", line 3, in <module>
  File builtin, in Int#//
TypeError: unsupported operand type\(s\) for //: 'Int' and 'Nil'
""", stderr)
            assert m is not None

        self._test("""
# int // nil (TypeError)
puts(42 // nil)
""", stderr=test_stderr)

    def test_floor_divide60(self):
        def test_stderr(stderr):
            m = match(r"""Traceback \(most recent call last\):
  File "[^"]+", line 3, in <module>
  File builtin, in Int#//
ZeroDivisionError: int division by zero
""", stderr)
            assert m is not None

        self._test("""
# int // zero (ZeroDivisionError)
puts(42 // 0)
""", stderr=test_stderr)

    def test_floor_divide70(self):
        def test_stderr(stderr):
            m = match(r"""Traceback \(most recent call last\):
  File "[^"]+", line 3, in <module>
  File builtin, in Int#//
ZeroDivisionError: float division
""", stderr)
            assert m is not None

        self._test("""
# int // 0.0 (ZeroDivisionError)
puts(42 // 0.0)
""", stderr=test_stderr)

    def test_positive0(self):
        self._test("""
puts(+ 42)
""", """42
""")

# vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
