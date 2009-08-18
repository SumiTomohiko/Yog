# -*- coding: utf-8 -*-

from testcase import TestCase

class TestOperator(TestCase):

    def test_assign0(self):
        self._test("""
foo = 42
print(foo += 26)
""", "68")

    def test_assign10(self):
        self._test("""
foo = 42
foo += 26
print(foo)
""", "68")

    def test_assign20(self):
        self._test("""
foo = 42
print(foo -= 26)
""", "16")

    def test_assign30(self):
        self._test("""
foo = 42
foo -= 26
print(foo)
""", "16")

    def test_assign40(self):
        self._test("""
foo = 42
print(foo *= 26)
""", "1092")

    def test_assign50(self):
        self._test("""
foo = 42
foo *= 26
print(foo)
""", "1092")

    def test_assign60(self):
        self._test("""
foo = 42
print(foo /= 26)
""", "1.61538")

    def test_assign70(self):
        self._test("""
foo = 42
foo /= 26
print(foo)
""", "1.61538")

    def test_assign80(self):
        self._test("""
foo = 42
print(foo //= 26)
""", "1")

    def test_assign90(self):
        self._test("""
foo = 42
foo //= 26
print(foo)
""", "1")

    def test_assign100(self):
        self._test("""
foo = 42
print(foo |= 26)
""", "58")

    def test_assign110(self):
        self._test("""
foo = 42
foo |= 26
print(foo)
""", "58")

    def test_assign120(self):
        self._test("""
foo = 42
print(foo &= 26)
""", "10")

    def test_assign130(self):
        self._test("""
foo = 42
foo &= 26
print(foo)
""", "10")

    def test_assign140(self):
        self._test("""
foo = 42
print(foo ^= 26)
""", "48")

    def test_assign150(self):
        self._test("""
foo = 42
foo ^= 26
print(foo)
""", "48")

    def test_assign160(self):
        self._test("""
foo = 42
print(foo *= 26)
""", "1601332619247764283850260201342556799238144")

    def test_assign170(self):
        self._test("""
foo = 42
foo *= 26
print(foo)
""", "1601332619247764283850260201342556799238144")

# vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
