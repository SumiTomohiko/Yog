# -*- coding: utf-8 -*-

from testcase import TestCase

class TestOperator(TestCase):

    def test_argumented_assign0(self):
        self._test("""
foo = 42
print(foo += 26)
""", "68")

    def test_argumented_assign10(self):
        self._test("""
foo = 42
foo += 26
print(foo)
""", "68")

    def test_argumented_assign20(self):
        self._test("""
foo = 42
print(foo -= 26)
""", "16")

    def test_argumented_assign30(self):
        self._test("""
foo = 42
foo -= 26
print(foo)
""", "16")

    def test_argumented_assign40(self):
        self._test("""
foo = 42
print(foo *= 26)
""", "1092")

    def test_argumented_assign50(self):
        self._test("""
foo = 42
foo *= 26
print(foo)
""", "1092")

    def test_argumented_assign60(self):
        self._test("""
foo = 42
print(foo /= 26)
""", "1.61538")

    def test_argumented_assign70(self):
        self._test("""
foo = 42
foo /= 26
print(foo)
""", "1.61538")

    def test_argumented_assign80(self):
        self._test("""
foo = 42
print(foo //= 26)
""", "1")

    def test_argumented_assign90(self):
        self._test("""
foo = 42
foo //= 26
print(foo)
""", "1")

    def test_argumented_assign100(self):
        self._test("""
foo = 42
print(foo |= 26)
""", "58")

    def test_argumented_assign110(self):
        self._test("""
foo = 42
foo |= 26
print(foo)
""", "58")

    def test_argumented_assign120(self):
        self._test("""
foo = 42
print(foo &= 26)
""", "10")

    def test_argumented_assign130(self):
        self._test("""
foo = 42
foo &= 26
print(foo)
""", "10")

    def test_argumented_assign140(self):
        self._test("""
foo = 42
print(foo ^= 26)
""", "48")

    def test_argumented_assign150(self):
        self._test("""
foo = 42
foo ^= 26
print(foo)
""", "48")

    def test_argumented_assign160(self):
        self._test("""
foo = 42
print(foo **= 26)
""", "1601332619247764283850260201342556799238144")

    def test_argumented_assign170(self):
        self._test("""
foo = 42
foo **= 26
print(foo)
""", "1601332619247764283850260201342556799238144")

    def test_argumented_assign180(self):
        self._test("""
foo = 42
print(foo %= 26)
""", "16")

    def test_argumented_assign190(self):
        self._test("""
foo = 42
foo %= 26
print(foo)
""", "16")

    def test_argumented_assign200(self):
        self._test("""
foo = 42
print(foo <<= 26)
""", "2818572288")

    def test_argumented_assign210(self):
        self._test("""
foo = 42
foo <<= 26
print(foo)
""", "2818572288")

    def test_argumented_assign210(self):
        self._test("""
foo = 42
print(foo >>= 26)
""", "0")

    def test_argumented_assign220(self):
        self._test("""
foo = 42
foo >>= 26
print(foo)
""", "0")

    def test_argumented_assign230(self):
        self._test("""
foo = false
print(foo &&= 42)
""", "false")

    def test_argumented_assign240(self):
        self._test("""
foo = false
foo &&= 42
print(foo)
""", "false")

    def test_argumented_assign250(self):
        self._test("""
foo = true
print(foo &&= 42)
""", "42")

    def test_argumented_assign260(self):
        self._test("""
foo = true
foo &&= 42
print(foo)
""", "42")

    def test_argumented_assign270(self):
        self._test("""
foo = false
print(foo ||= 42)
""", "42")

    def test_argumented_assign280(self):
        self._test("""
foo = false
foo ||= 42
print(foo)
""", "42")

    def test_argumented_assign290(self):
        self._test("""
foo = true
print(foo ||= 42)
""", "true")

    def test_argumented_assign300(self):
        self._test("""
foo = true
foo ||= 42
print(foo)
""", "true")

# vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
