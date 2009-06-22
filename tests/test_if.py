# -*- coding: utf-8 -*-

from testcase import TestCase

class TestIf(TestCase):

    def test_true(self):
        self._test("""
if 0 < 1
    puts(42)
else
    puts(0)
end
""", "42\n")

    def test_false(self):
        self._test("""
if 1 < 0
    puts(0)
else
    puts(42)
end
""", "42\n")

    def test_empty_statements10(self):
        self._test("""
if true
  puts(42)
else
end""", """42
""")

    def test_empty_statements20(self):
        self._test("""
if false
else
  puts(42)
end""", """42
""")

# vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
