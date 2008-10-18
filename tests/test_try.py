# -*- coding: utf-8 -*-

from tests import TestCase

class TestTry(TestCase):

    def test_except(self):
        self._test("""
try
    raise 0
except
    puts 42
end""", "42\n")

    def test_break_in_finally1(self):
        self._test("""
i = 0
while i < 10
    try
        break
    finally
        puts 42
    end
end
""", "42\n")

    def test_break_in_finally2(self):
        self._test("""
i = 0
while i < 10
    try
        try
            break
        finally
            puts 42
        end
    finally
        puts 43
    end
end
""", """42
43
""")

# vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
