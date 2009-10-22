# -*- coding: utf-8 -*-

from testcase import TestCase

class TestCoroutine(TestCase):

    def test_resume0(self):
        self._test("""
co = Coroutine.new() do
  print(42)
end
co.resume()
""", "42")

    def test_yield0(self):
        self._test("""
co = Coroutine.new() do
  print(42)
  Coroutine.yield()
  print(44)
end
co.resume()
print(43)
co.resume()
print(45)
""", "42434445")

    def test_yield10(self):
        self._test("""
co = Coroutine.new() do
  26.times() do
    print(42)
    Coroutine.yield()
  end
end

26.times() do
  co.resume()
end
""", "4242424242424242424242424242424242424242424242424242")

# vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
