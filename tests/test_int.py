# -*- coding: utf-8 -*-

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
puts(0b_)
""", stderr="""puts(0b2)
       ^
SyntaxError: numeric literal without digits
""")

    def test_binary_literal60(self):
        self._test("""
puts(0b_)
""", stderr="""puts(0b1_2)
         ^
SyntaxError: numeric literal without digits
""")

# vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
