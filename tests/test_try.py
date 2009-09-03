# -*- coding: utf-8 -*-

from re import match
from testcase import TestCase

class TestTry(TestCase):

    def test_except1(self):
        self._test("""
try
    raise(0)
except
    puts(42)
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
            puts(100)
        finally
            raise(0)
        end
    except
        puts(42)
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
        puts(42)
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
            puts(42)
        end
    finally
        puts(43)
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
        puts(42)
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
            puts(42)
        end
    finally
        puts(43)
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
  puts(42)
end""")

    def test_longjmp_from_native_function0(self):
        self._test("""
# http://bitbucket.org/SumiTomohiko/yog/issue/2/
File.open("foo.txt", "r") do [f]
  try
    while true
      line = f.readline()
    end
  except
  end
end
""", "")

    def test_except_type0(self):
        self._test("""
try
  raise Exception.new()
except Exception
  print(42)
end
""", "42")

    def test_except_type10(self):
        self._test("""
class FooException > Exception
end

try
  raise FooException.new()
except FooException
  print(42)
end
""", "42")

    def test_except_type20(self):
        self._test("""
class FooException > Exception
end

class BarException > Exception
end

try
  raise FooException.new()
except FooException, BarException
  print(42)
end
""", "42")

    def test_except_type30(self):
        self._test("""
class FooException > Exception
end

class BarException > Exception
end

try
  raise BarException.new()
except FooException, BarException
  print(42)
end
""", "42")

    def test_except_type40(self):
        self._test("""
class FooException > Exception
end

try
  raise Exception.new()
except FooException
  print(42)
except
  print(26)
end
""", "26")

    def test_except_type50(self):
        self._test("""
class FooException > Exception
end

try
  raise FooException.new(42)
except FooException as e
  print(e.message)
end
""", "42")

    def test_except_type60(self):
        def test_stderr(stderr):
            m = match(r"""Traceback \(most recent call last\):
  File "[^"]+", line 6, in <package>
Exception: 42
""", stderr)
            assert m is not None

        self._test("""
class FooException > Exception
end

try
  raise Exception.new(42)
except FooException
  print(26)
end
""", stderr=test_stderr)

# vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
