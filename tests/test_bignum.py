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
puts(4611686018427387903 + 1)
""", """4611686018427387904
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

# vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
