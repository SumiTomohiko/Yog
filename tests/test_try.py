# -*- coding: utf-8 -*-

from testcase import TestCase

class TestTry(TestCase):

    def test_except1(self):
        self._test("""
try
    raise 0
except
    puts 42
end""", "42\n", "")

    def test_except2(self):
        self._test("""
i = 0
while i < 10
    i = i + 1

    try
        try
            break
        except
            puts 100
        finally
            raise 0
        end
    except
        puts 42
    end
end""", """42
42
42
42
42
42
42
42
42
42
""", "")

    def test_break_in_finally1(self):
        self._test("""
i = 0
while i < 10
    i = i + 1

    try
        break
    finally
        puts 42
    end
end
""", """42
""", "")

    def test_break_in_finally2(self):
        self._test("""
i = 0
while i < 10
    i = i + 1

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
""", "")

    def test_next_in_finally1(self):
        self._test("""
i = 0
while i < 10
    i = i + 1

    try
        next
    finally
        puts 42
    end
end
""", """42
42
42
42
42
42
42
42
42
42
""", "")

    def test_next_in_finally2(self):
        self._test("""
i = 0
while i < 10
    i = i + 1

    try
        try
            next
        finally
            puts 42
        end
    finally
        puts 43
    end
end
""", """42
43
42
43
42
43
42
43
42
43
42
43
42
43
42
43
42
43
42
43
""", "")

    def test_blank_try(self):
        self._test("""
try
except
  puts 42
end""")

# vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
