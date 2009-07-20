# -*- coding: utf-8 -*-

from testcase import TestCase

class TestInteractive(TestCase):

    def test_interactive0(self):
        self._test(stdout=""">>> 42
>>> """, stdin="""puts(42)
""")

    def test_interactive10(self):
        self._test(stdout=""">>> 42
>>> 26
>>> """, stdin="""puts(42)
puts(26)
""")

# vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
