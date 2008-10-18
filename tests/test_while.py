# -*- coding: utf-8 -*-

from tests import TestCase

class TestWhile(TestCase):

    def test_while1(self):
        self._test("""
i = 0
while i < 1
    puts i
    i = i + 1
end""", """0
""")

    def test_while2(self):
        self._test("""
i = 0
while i < 2
    puts i
    i = i + 1
end""", """0
1
""")

    def test_while3(self):
        self._test("""
i = 0
while i < 0
    puts i
    i = i + 1
end""", "")

    def test_break(self):
        self._test("""
i = 0
while i < 100
    if 10 < i
        break
    end
    puts i
    i = i + 1
end
puts 42""", """0
1
2
3
4
5
6
7
8
9
10
42
""")

    def test_next(self):
        self._test("""
i = 0
while i < 10
    i = i + 1
    if i < 5
        next
    end
    puts i
end
puts 42""", """5
6
7
8
9
42
""")

# vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
