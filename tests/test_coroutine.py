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

# vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
