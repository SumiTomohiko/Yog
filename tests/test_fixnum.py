# -*- coding: utf-8 -*-

from testcase import TestCase, enumerate_tuples

class TestFixnum(TestCase):

    def test_to0(self):
        self._test("print(26.to(42, &nop))", "26")

    def test_to10(self):
        self._test("26.to(42, &print)", "26272829303132333435363738394041")

    def test_to20(self):
        self._test("26.to(42, 2, &print)", "2628303234363840")

    def test_to30(self):
        self._test("42.to(26, &print)")

    def test_to40(self):
        self._test("42.to(26, -1, &print)", "42414039383736353433323130292827")

    def test_to50(self):
        def test_stderr(stderr):
            assert 0 < stderr.find("ValueError: step must be not zero")
        self._test("26.to(42, 0, &nop)", stderr=test_stderr)

    def test_plus_one(self):
        self._test("""
puts(42 + 1)
""", "43\n")

    def test_equal0(self):
        self._test("""
print(42 == 26)
""", "false")

    def test_equal10(self):
        self._test("""
print(42 == 42)
""", "true")

    def test_less0(self):
        self._test("""
puts(0 < 42)
""", "true\n")

    def test_less10(self):
        self._test("""
puts(42 < 0)
""", "false\n")

    def test_less_equal0(self):
        self._test("""
print(42 <= 26)
""", "false")

    def test_less_equal10(self):
        self._test("""
print(26 <= 42)
""", "true")

    def test_less_equal20(self):
        self._test("""
print(42 <= 42)
""", "true")

    def test_greater0(self):
        self._test("""
print(42 > 26)
""", "true")

    def test_greater10(self):
        self._test("""
print(26 > 42)
""", "false")

    def test_greater_equal0(self):
        self._test("""
print(26 >= 42)
""", "false")

    def test_greater_equal10(self):
        self._test("""
print(42 >= 26)
""", "true")

    def test_greater_equal20(self):
        self._test("""
print(42 >= 42)
""", "true")

    def test_less20(self):
        def test_stderr(stderr):
            self._test_regexp(r"""Traceback \(most recent call last\):
  File "[^"]+", line 2, in <package>
TypeError: Comparison of Fixnum with Nil failed
""", stderr)

        self._test("""
puts(42 < nil)
""", stderr=test_stderr)

    for i, num, radix, expected in enumerate_tuples((
        (42, None, "42"),
        (42, 2, "101010"),
        (42, 8, "52"),
        (42, 10, "42"),
        (42, 16, "2a"),
        (42, 36, "16"))):
        fmt = """def test_to_s{index}(self):
    self._test(\"print({num}.to_s({radix}))\", \"{expected}\")"""
        exec(fmt.format(
            index=10 * i,
            num=num,
            radix=radix if radix is not None else "",
            expected=expected))

    def test_times(self):
        self._test("""
10.times() do |n|
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
SyntaxError: Trailing \"_\" in number
""")

    def test_binary_literal40(self):
        self._test("""
puts(0b_)
""", stderr="""puts(0b_)
       ^
SyntaxError: Numeric literal without digits
""")

    def test_binary_literal50(self):
        self._test("""
puts(0b2)
""", stderr="""puts(0b2)
       ^
SyntaxError: Numeric literal without digits
""")

    def test_binary_literal60(self):
        self._test("""
puts(0b1_2)
""", stderr="""puts(0b1_2)
         ^
SyntaxError: Numeric literal without digits
""")

    def test_digit_literal0(self):
        self._test("""
puts(0d42)
""", """42
""")

    def test_digit_literal10(self):
        self._test("""
puts(0D42)
""", """42
""")

    def test_digit_literal13(self):
        self._test("""
puts(0d0)
""", """0
""")

    def test_digit_literal16(self):
        self._test("""
puts(0d9)
""", """9
""")

    def test_digit_literal20(self):
        self._test("""
puts(0d4_2)
""", """42
""")

    def test_digit_literal30(self):
        self._test("""
puts(0d4__2)
""", stderr="""puts(0d4__2)
         ^
SyntaxError: Trailing \"_\" in number
""")

    def test_digit_literal40(self):
        self._test("""
puts(0d_)
""", stderr="""puts(0d_)
       ^
SyntaxError: Numeric literal without digits
""")

    def test_digit_literal50(self):
        self._test("""
puts(0da)
""", stderr="""puts(0da)
       ^
SyntaxError: Numeric literal without digits
""")

    def test_digit_literal60(self):
        self._test("""
puts(0d4_a)
""", stderr="""puts(0d4_a)
         ^
SyntaxError: Numeric literal without digits
""")

    def test_octet_literal0(self):
        self._test("""
puts(0o42)
""", """34
""")

    def test_octet_literal10(self):
        self._test("""
puts(0O42)
""", """34
""")

    def test_octet_literal13(self):
        self._test("""
puts(0o0)
""", """0
""")

    def test_octet_literal16(self):
        self._test("""
puts(0o7)
""", """7
""")

    def test_octet_literal20(self):
        self._test("""
puts(0o4_2)
""", """34
""")

    def test_octet_literal30(self):
        self._test("""
puts(0o4__2)
""", stderr="""puts(0o4__2)
         ^
SyntaxError: Trailing \"_\" in number
""")

    def test_octet_literal40(self):
        self._test("""
puts(0o_)
""", stderr="""puts(0o_)
       ^
SyntaxError: Numeric literal without digits
""")

    def test_octet_literal50(self):
        self._test("""
puts(0o8)
""", stderr="""puts(0o8)
       ^
SyntaxError: Numeric literal without digits
""")

    def test_octet_literal60(self):
        self._test("""
puts(0o4_8)
""", stderr="""puts(0o4_8)
         ^
SyntaxError: Numeric literal without digits
""")

    def test_hex_literal0(self):
        self._test("""
puts(0x42)
""", """66
""")

    def test_hex_literal10(self):
        self._test("""
puts(0X42)
""", """66
""")

    def test_hex_literal20(self):
        self._test("""
puts(0x0)
""", """0
""")

    def test_hex_literal30(self):
        self._test("""
puts(0x9)
""", """9
""")

    def test_hex_literal40(self):
        self._test("""
puts(0xa)
""", """10
""")

    def test_hex_literal50(self):
        self._test("""
puts(0xf)
""", """15
""")

    def test_hex_literal60(self):
        self._test("""
puts(0xA)
""", """10
""")

    def test_hex_literal70(self):
        self._test("""
puts(0xF)
""", """15
""")

    def test_hex_literal80(self):
        self._test("""
puts(0x4_2)
""", """66
""")

    def test_hex_literal90(self):
        self._test("""
puts(0x4__2)
""", stderr="""puts(0x4__2)
         ^
SyntaxError: Trailing \"_\" in number
""")

    def test_hex_literal100(self):
        self._test("""
puts(0x_)
""", stderr="""puts(0x_)
       ^
SyntaxError: Numeric literal without digits
""")

    def test_hex_literal110(self):
        self._test("""
puts(0xg)
""", stderr="""puts(0xg)
       ^
SyntaxError: Numeric literal without digits
""")

    def test_hex_literal120(self):
        self._test("""
puts(0xG)
""", stderr="""puts(0xG)
       ^
SyntaxError: Numeric literal without digits
""")

    def test_hex_literal130(self):
        self._test("""
puts(0x4_g)
""", stderr="""puts(0x4_g)
         ^
SyntaxError: Numeric literal without digits
""")

    def test_hex_literal140(self):
        self._test("""
puts(0x4_G)
""", stderr="""puts(0x4_G)
         ^
SyntaxError: Numeric literal without digits
""")

    def test_add0(self):
        self._test("""
puts(42 + 26)
""", """68
""")

    def test_add10(self):
        self._test("""
# Fixnum + Fixnum = Bignum (32bit)
puts(1 + 1073741823)
""", """1073741824
""")

    def test_add20(self):
        self._test("""
# Fixnum + Fixnum = Bignum (64bit)
puts(1 + 4611686018427387903)
""", """4611686018427387904
""")

    def test_add30(self):
        self._test("""
# Fixnum + Bignum (32bit)
puts(1 + 1073741824)
""", """1073741825
""")

    def test_add40(self):
        self._test("""
# Fixnum + Bignum (64bit)
puts(1 + 4611686018427387904)
""", """4611686018427387905
""")

    def test_add60(self):
        def test_stderr(stderr):
            self._test_regexp(r"""Traceback \(most recent call last\):
  File "[^"]+", line 3, in <package>
TypeError: unsupported operand type\(s\) for \+: Fixnum and Object
""", stderr)

        self._test("""
# Fixnum + Object (TypeError)
puts(42 + Object.new())
""", stderr=test_stderr)

    def test_add70(self):
        def test_stderr(stderr):
            self._test_regexp(r"""Traceback \(most recent call last\):
  File "[^"]+", line 3, in <package>
TypeError: unsupported operand type\(s\) for \+: Fixnum and Nil
""", stderr)

        self._test("""
# Fixnum + Nil (TypeError)
puts(42 + nil)
""", stderr=test_stderr)

    def test_add80(self):
        def test_stderr(stderr):
            self._test_regexp(r"""Traceback \(most recent call last\):
  File "[^"]+", line 3, in <package>
TypeError: unsupported operand type\(s\) for \+: Fixnum and Bool
""", stderr)

        self._test("""
# Fixnum + Bool (TypeError)
puts(42 + true)
""", stderr=test_stderr)

    def test_add90(self):
        def test_stderr(stderr):
            self._test_regexp(r"""Traceback \(most recent call last\):
  File "[^"]+", line 3, in <package>
TypeError: unsupported operand type\(s\) for \+: Fixnum and Symbol
""", stderr)

        self._test("""
# Fixnum + Symbol (TypeError)
puts(42 + 'foo)
""", stderr=test_stderr)

    def test_negative0(self):
        self._test("""
puts(- 42)
""", """-42
""")

    def test_subtract0(self):
        self._test("""
# Fixnum - Fixnum = Fixnum
puts(42 - 26)
""", """16
""")

    def test_subtract10(self):
        self._test("""
# Fixnum - Fixnum = Bignum (32bit)
puts(- 1 - 1073741823)
""", """-1073741824
""")

    def test_subtract20(self):
        self._test("""
# Fixnum - Fixnum = Bignum (64bit)
puts(- 1 - 4611686018427387903)
""", """-4611686018427387904
""")

    def test_subtract30(self):
        self._test("""
# Fixnum - Bignum (32bit)
puts(- 1 - 1073741824)
""", """-1073741825
""")

    def test_subtract40(self):
        self._test("""
# Fixnum - Bignum (64bit)
puts(- 1 - 4611686018427387904)
""", """-4611686018427387905
""")

    def test_subtract50(self):
        self._test("""
# Fixnum - Float
puts(- 42 - 3.141592)
""", """-45.141592
""")

    def test_subtract60(self):
        def test_stderr(stderr):
            self._test_regexp(r"""Traceback \(most recent call last\):
  File "[^"]+", line 3, in <package>
TypeError: unsupported operand type\(s\) for -: Fixnum and Object
""", stderr)

        self._test("""
# Fixnum - Object (TypeError)
puts(42 - Object.new())
""", stderr=test_stderr)

    def test_subtract70(self):
        def test_stderr(stderr):
            self._test_regexp(r"""Traceback \(most recent call last\):
  File "[^"]+", line 3, in <package>
TypeError: unsupported operand type\(s\) for -: Fixnum and Nil
""", stderr)

        self._test("""
# Fixnum - Nil (TypeError)
puts(42 - nil)
""", stderr=test_stderr)

    def test_subtract80(self):
        def test_stderr(stderr):
            self._test_regexp(r"""Traceback \(most recent call last\):
  File "[^"]+", line 3, in <package>
TypeError: unsupported operand type\(s\) for -: Fixnum and Bool
""", stderr)

        self._test("""
# Fixnum - Bool (TypeError)
puts(42 - true)
""", stderr=test_stderr)

    def test_subtract90(self):
        def test_stderr(stderr):
            self._test_regexp(r"""Traceback \(most recent call last\):
  File "[^"]+", line 3, in <package>
TypeError: unsupported operand type\(s\) for -: Fixnum and Symbol
""", stderr)

        self._test("""
# Fixnum - Symbol (TypeError)
puts(42 - 'foo)
""", stderr=test_stderr)

    def test_multiply0(self):
        self._test("""
# Fixnum * Fixnum = Fixnum
puts(26 * 42)
""", """1092
""")

    def test_multiply10(self):
        self._test("""
# Fixnum * Fixnum = Bignum (32bit)
puts(2 * 536870912)
""", """1073741824
""")

    def test_multiply20(self):
        self._test("""
# Fixnum * Fixnum = Bignum (64bit)
puts(2 * 2305843009213693952)
""", """4611686018427387904
""")

    def test_multiply30(self):
        self._test("""
# Fixnum * Bignum = Bignum
puts(2 * 4611686018427387904)
""", """9223372036854775808
""")

    def test_multiply40(self):
        self._test("""
# Fixnum * float = float
puts(2 * 3.1415926535)
""", """6.283185307
""")

    def test_multiply45(self):
        self._test("print(0 * 42)", "0")

    def test_multiply50(self):
        def test_stderr(stderr):
            self._test_regexp(r"""Traceback \(most recent call last\):
  File "[^"]+", line 3, in <package>
TypeError: unsupported operand type\(s\) for \*: Fixnum and Bool
""", stderr)

        self._test("""
# Fixnum * bool (TypeError)
puts(42 * true)
""", stderr=test_stderr)

    def test_multiply60(self):
        def test_stderr(stderr):
            self._test_regexp(r"""Traceback \(most recent call last\):
  File "[^"]+", line 3, in <package>
TypeError: unsupported operand type\(s\) for \*: Fixnum and Nil
""", stderr)

        self._test("""
# Fixnum * nil (TypeError)
puts(42 * nil)
""", stderr=test_stderr)

    def test_multiply70(self):
        def test_stderr(stderr):
            self._test_regexp(r"""Traceback \(most recent call last\):
  File "[^"]+", line 3, in <package>
TypeError: unsupported operand type\(s\) for \*: Fixnum and Object
""", stderr)

        self._test("""
# Fixnum * Object (TypeError)
puts(42 * Object.new())
""", stderr=test_stderr)

    def test_multiply80(self):
        def test_stderr(stderr):
            self._test_regexp(r"""Traceback \(most recent call last\):
  File "[^"]+", line 3, in <package>
TypeError: unsupported operand type\(s\) for \*: Fixnum and Symbol
""", stderr)

        self._test("""
# Fixnum * Symbol (TypeError)
puts(42 * 'foo)
""", stderr=test_stderr)

    def test_divide0(self):
        self._test("""
# Fixnum / Fixnum
puts(42 / 26)
""", """1.61538461538
""")

    def test_divide10(self):
        self._test("""
# Fixnum / float
puts(42 / 3.1415926535)
""", """13.3690152201
""")

    def test_divide20(self):
        self._test("""
# Fixnum / Bignum (32bit)
puts(536870912 / 1073741824)
""", """0.5
""")

    def test_divide30(self):
        # TODO: enable this test
        return

        self._test("""
# Fixnum / Bignum (64bit)
puts(2305843009213693952 / 4611686018427387904)
""", """0.5
""")

    def test_divide40(self):
        def test_stderr(stderr):
            self._test_regexp(r"""Traceback \(most recent call last\):
  File "[^"]+", line 3, in <package>
TypeError: unsupported operand type\(s\) for /: Fixnum and Bool
""", stderr)

        self._test("""
# Fixnum / bool (TypeError)
puts(42 / true)
""", stderr=test_stderr)

    def test_divide50(self):
        def test_stderr(stderr):
            self._test_regexp(r"""Traceback \(most recent call last\):
  File "[^"]+", line 3, in <package>
TypeError: unsupported operand type\(s\) for /: Fixnum and Nil
""", stderr)

        self._test("""
# Fixnum / nil (TypeError)
puts(42 / nil)
""", stderr=test_stderr)

    def test_divide60(self):
        def test_stderr(stderr):
            self._test_regexp(r"""Traceback \(most recent call last\):
  File "[^"]+", line 3, in <package>
ZeroDivisionError: Fixnum division by zero
""", stderr)

        self._test("""
# Fixnum / zero (ZeroDivisionError)
puts(42 / 0)
""", stderr=test_stderr)

    def test_divide70(self):
        def test_stderr(stderr):
            self._test_regexp(r"""Traceback \(most recent call last\):
  File "[^"]+", line 3, in <package>
ZeroDivisionError: Float division
""", stderr)

        self._test("""
# Fixnum / 0.0 (ZeroDivisionError)
puts(42 / 0.0)
""", stderr=test_stderr)

    def test_floor_divide0(self):
        self._test("""
# Fixnum // Fixnum
puts(42 // 26)
""", """1
""")

    def test_floor_divide10(self):
        self._test("""
# Fixnum // float
puts(42 // 3.1415926535)
""", """13.3690152201
""")

    def test_floor_divide20(self):
        self._test("""
# Fixnum // Bignum (32bit)
puts(536870912 // 1073741824)
""", """0
""")

    def test_floor_divide30(self):
        # TODO: enable this test
        return

        self._test("""
# Fixnum // Bignum (64bit)
puts(2305843009213693952 // 4611686018427387904)
""", """0
""")

    def test_floor_divide31(self):
        self._test("print((-42) // 26)", "-2")

    def test_floor_divide32(self):
        self._test("print(42 // (-26))", "-2")

    def test_floor_divide33(self):
        self._test("print((-42) // (-26))", "1")

    def test_floor_divide34(self):
        self._test("print((-42) // 4611686018427387904)", "-1")

    def test_floor_divide35(self):
        self._test("print(42 // (-4611686018427387904))", "-1")

    def test_floor_divide36(self):
        self._test("print((-42) // (-4611686018427387904))", "0")

    def test_floor_divide40(self):
        def test_stderr(stderr):
            self._test_regexp(r"""Traceback \(most recent call last\):
  File "[^"]+", line 3, in <package>
TypeError: unsupported operand type\(s\) for //: Fixnum and Bool
""", stderr)

        self._test("""
# Fixnum // bool (TypeError)
puts(42 // true)
""", stderr=test_stderr)

    def test_floor_divide50(self):
        def test_stderr(stderr):
            self._test_regexp(r"""Traceback \(most recent call last\):
  File "[^"]+", line 3, in <package>
TypeError: unsupported operand type\(s\) for //: Fixnum and Nil
""", stderr)

        self._test("""
# Fixnum // nil (TypeError)
puts(42 // nil)
""", stderr=test_stderr)

    def test_floor_divide60(self):
        def test_stderr(stderr):
            self._test_regexp(r"""Traceback \(most recent call last\):
  File "[^"]+", line 3, in <package>
ZeroDivisionError: Fixnum division by zero
""", stderr)

        self._test("""
# Fixnum // zero (ZeroDivisionError)
puts(42 // 0)
""", stderr=test_stderr)

    def test_floor_divide70(self):
        def test_stderr(stderr):
            self._test_regexp(r"""Traceback \(most recent call last\):
  File "[^"]+", line 3, in <package>
ZeroDivisionError: float division
""", stderr)

        self._test("""
# Fixnum // 0.0 (ZeroDivisionError)
puts(42 // 0.0)
""", stderr=test_stderr)

    def test_positive0(self):
        self._test("""
puts(+ 42)
""", """42
""")

    def test_left_shift0(self):
        self._test("""
puts(42 << 0)
""", """42
""")

    def test_left_shift10(self):
        self._test("""
# Fixnum << Fixnum = Fixnum (32bit)
puts(536870911 << 1)
""", """1073741822
""")

    def test_left_shift20(self):
        self._test("""
# Fixnum << Fixnum = Fixnum (64bit)
puts(2305843009213693951 << 1)
""", """4611686018427387902
""")

    def test_left_shift30(self):
        self._test("""
# Fixnum << Fixnum = Bignum (32bit)
puts(536870912 << 1)
""", """1073741824
""")

    def test_left_shift40(self):
        self._test("""
# Fixnum << Fixnum = Bignum (64bit)
puts(2305843009213693952 << 1)
""", """4611686018427387904
""")

    def test_left_shift50(self):
        self._test("""
# Fixnum << Fixnum (negative) = Fixnum
puts(42 << (- 1))
""", """21
""")

    def test_left_shift60(self):
        self._test("""
# Fixnum (negative, odd number) << Fixnum (negative) = Fixnum
puts((- 3) << (- 1))
""", """-2
""")

    def test_left_shift70(self):
        def test_stderr(stderr):
            self._test_regexp(r"""Traceback \(most recent call last\):
  File "[^"]+", line 2, in <package>
TypeError: unsupported operand type\(s\) for <<: Fixnum and String
""", stderr)

        self._test("""
puts(42 << "foo")
""", stderr=test_stderr)

    def test_right_shift0(self):
        self._test("""
puts(42 >> 0)
""", """42
""")

    def test_right_shift10(self):
        self._test("""
# Fixnum >> Fixnum = Fixnum
puts(1073741823 >> 1)
""", """536870911
""")

    def test_right_shift20(self):
        self._test("""
# Fixnum >> Fixnum = zero (32bit)
puts(1 >> 32)
""", """0
""")

    def test_right_shift30(self):
        self._test("""
# Fixnum >> Fixnum = zero (64bit)
puts(1 >> 64)
""", """0
""")

    def test_right_shift40(self):
        self._test("""
# Fixnum >> Fixnum (negative) = Fixnum (32bit)
puts(536870911 >> (- 1))
""", """1073741822
""")

    def test_right_shift50(self):
        self._test("""
# Fixnum >> Fixnum (negative) = Fixnum (64bit)
puts(2305843009213693951 >> (- 1))
""", """4611686018427387902
""")

    def test_right_shift60(self):
        self._test("""
# Fixnum >> Fixnum (negative) = Bignum (32bit)
puts(536870912 >> (- 1))
""", """1073741824
""")

    def test_right_shift70(self):
        self._test("""
# Fixnum >> Fixnum (negative) = Bignum (64bit)
puts(2305843009213693952 >> (- 1))
""", """4611686018427387904
""")

    def test_right_shift80(self):
        self._test("""
# Fixnum (negative, odd number) >> Fixnum = Fixnum
puts((- 3) >> 1)
""", """-2
""")

    def test_right_shift90(self):
        def test_stderr(stderr):
            self._test_regexp(r"""Traceback \(most recent call last\):
  File "[^"]+", line 2, in <package>
TypeError: unsupported operand type\(s\) for >>: Fixnum and String
""", stderr)

        self._test("""
puts(42 >> "foo")
""", stderr=test_stderr)

    def test_bitwise_or0(self):
        self._test("""
# Fixnum | Fixnum
puts(42 | 26)
""", """58
""")

    def test_bitwise_or10(self):
        self._test("""
# Fixnum | Bignum
puts(42 | 4611686018427387904)
""", """4611686018427387946
""")

    def test_bitwise_or20(self):
        def test_stderr(stderr):
            self._test_regexp(r"""Traceback \(most recent call last\):
  File "[^"]+", line 2, in <package>
TypeError: unsupported operand type\(s\) for \|: Fixnum and String
""", stderr)

        self._test("""
puts(42 | "foo")
""", stderr=test_stderr)

    def test_bitwise_and0(self):
        self._test("""
# Fixnum & Fixnum
puts(42 & 26)
""", """10
""")

    def test_bitwise_and10(self):
        self._test("""
# Fixnum & Bignum
puts(42 & 4611686018427387904)
""", """0
""")

    def test_bitwise_and20(self):
        def test_stderr(stderr):
            self._test_regexp(r"""Traceback \(most recent call last\):
  File "[^"]+", line 2, in <package>
TypeError: unsupported operand type\(s\) for &: Fixnum and String
""", stderr)

        self._test("""
puts(42 & "foo")
""", stderr=test_stderr)

    def test_xor0(self):
        self._test("""
puts(42 ^ 26)
""", """48
""")

    def test_xor10(self):
        self._test("""
puts(42 ^ 1073741824)
""", """1073741866
""")

    def test_xor20(self):
        def test_stderr(stderr):
            self._test_regexp(r"""Traceback \(most recent call last\):
  File "[^"]+", line 2, in <package>
TypeError: unsupported operand type\(s\) for \^: Fixnum and String
""", stderr)

        self._test("""
puts(42 ^ "foo")
""", stderr=test_stderr)

    def test_modulo0(self):
        self._test("""
# Fixnum % Fixnum
puts(42 % 26)
""", """16
""")

    def test_modulo10(self):
        self._test("""
# Fixnum % Bignum
puts(42 % 4611686018427387904)
""", """42
""")

    def test_modulo20(self):
        def test_stderr(stderr):
            self._test_regexp(r"""Traceback \(most recent call last\):
  File "[^"]+", line 2, in <package>
TypeError: unsupported operand type\(s\) for %: Fixnum and String
""", stderr)

        self._test("""
puts(42 % "foo")
""", stderr=test_stderr)

    def test_not0(self):
        self._test("""
puts(~ 42)
""", """-43
""")

    def test_power0(self):
        self._test("""
# Fixnum ** Fixnum
print(2 ** 3)
""", "8")

    def test_power10(self):
        self._test("""
# Fixnum ** Fixnum
print(0 ** 42)
""", "0")

    def test_power20(self):
        self._test("""
# Fixnum ** Fixnum
print(1 ** 42)
""", "1")

    def test_power30(self):
        self._test("""
# Fixnum ** Fixnum
print((- 1) ** 42)
""", "1")

    def test_power40(self):
        self._test("""
# Fixnum ** Fixnum
print((- 1) ** 3)
""", "-1")

    def test_power50(self):
        def test_stdout(stdout):
            self._test_regexp(r"6\.24479878809e-0*43", stdout)

        self._test("""
# Fixnum ** Fixnum
print(42 ** (- 26))
""", stdout=test_stdout)

    def test_power60(self):
        self._test("""
# Fixnum ** Fixnum
print(42 ** 0)
""", "1")

    def test_power70(self):
        self._test("""
# Fixnum ** Fixnum
print(42 ** 1)
""", "42")

    def test_power80(self):
        self._test("""
# Fixnum ** Fixnum = Bignum
print(42 ** 26)
""", "1601332619247764283850260201342556799238144")

    def test_power90(self):
        self._test("""
# Fixnum ** Float
print(42 ** 3.1415926535)
""", "125773.181137")

    def test_power100(self):
        self._test("""
# Fixnum ** Float
print(0 ** 3.1415926535)
""", "0.0")

    def test_power110(self):
        self._test("""
# Fixnum ** Float
print(1 ** 3.1415926535)
""", "1.0")

    def test_power120(self):
        self._test("""
# Fixnum ** Float
print((- 1) ** 3.1415926535)
""", "NaN")

    def test_power130(self):
        def test_stdout(stdout):
            self._test_regexp(r"7\.9508206039e-0*6", stdout)

        self._test("""
# Fixnum ** Float
print(42 ** (- 3.1415926535))
""", stdout=test_stdout)

    def test_power140(self):
        self._test("""
# Fixnum ** Float
print(42 ** 0.0)
""", "1.0")

    def test_power150(self):
        def test_stderr(stderr):
            self._test_regexp(r"""Traceback \(most recent call last\):
  File "[^"]+", line 2, in <package>
ZeroDivisionError: 0.0 cannot be raised to a negative power
""", stderr)

        self._test("""
print(0 ** (- 1))
""", stderr=test_stderr)

    def test_power160(self):
        def test_stderr(stderr):
            self._test_regexp(r"""Traceback \(most recent call last\):
  File "[^"]+", line 2, in <package>
TypeError: unsupported operand type\(s\) for \*\*: Fixnum and Bool
""", stderr)

        self._test("""
print(42 ** false)
""", stderr=test_stderr)

# vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
