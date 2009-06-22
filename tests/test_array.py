# -*- coding: utf-8 -*-

from testcase import TestCase

class TestArray(TestCase):

    def test_literal1(self):
        self._test("""
a = [42]
puts(a[0])
""", """42
""")

    def test_literal2(self):
        self._test("""
a = [42, 26]
puts(a[1])
""", """26
""")

    def test_literal3(self):
        self._test("""
a = []
""", "")

    def test_lshift1(self):
        self._test("""
a = []
a << 42
puts(a[0])
""", """42
""")

    def test_each1(self):
        self._test("""
a = []
a.each() [n]
  puts(n)
end
""", "")

    def test_each2(self):
        self._test("""
a = [42]
a.each() [n]
  puts(n)
end
""", """42
""")

    def test_each3(self):
        self._test("""
a = [42, 26]
a.each() [n]
  puts(n)
end
""", """42
26
""")

# vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
