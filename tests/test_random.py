# -*- coding: utf-8 -*-

from testcase import TestCase

class TestRandom(TestCase):

    options = []

    def test_seed0(self):
        self._test("""from random import seed
seed(42)""")

    def test_random0(self):
        n = 1024
        self._test("""from random import random
{n}.times() do
  print(random(0))
end""".format(**locals()), n * "0")

    def test_random10(self):
        n = 1024
        self._test("""from random import random
{n}.times() do
  print(random(42, 42))
end""".format(**locals()), n * "42")

# vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
