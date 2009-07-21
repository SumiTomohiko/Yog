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

    def test_interactive20(self):
        self._test(stdout=""">>> foobar
>>> >>> """, stderr="""Traceback (most recent call last):
  File \"__main__\", in <module>
  File builtin, in String#+
TypeError: can't convert 'Int' object to string implicitly
""", stdin="""puts(\"foo\" + \"bar\")
puts(\"foo\" + 42)
""")

    def test_interactive30(self):
        self._test(stdout=">>> >>> >>> ", stderr="""Traceback (most recent call last):
  File \"__main__\", in <module>
  File builtin, in String#+
TypeError: can't convert 'Int' object to string implicitly
Traceback (most recent call last):
  File \"__main__\", in <module>
  File builtin, in String#+
TypeError: can't convert 'Int' object to string implicitly
""", stdin="""puts(\"foo\" + 42)
puts(\"foo\" + 42)
""")

# vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
