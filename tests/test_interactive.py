# -*- coding: utf-8 -*-

from testcase import TestCase

class TestInteractive(TestCase):

    def test_interactive0(self):
        self._test(stdout=""">>> 42
=> nil
>>> """, stdin="""puts(42)
""")

    def test_interactive10(self):
        self._test(stdout=""">>> 42
=> nil
>>> 26
=> nil
>>> """, stdin="""puts(42)
puts(26)
""")

    def test_interactive20(self):
        self._test(stdout=""">>> foobar
=> nil
>>> >>> """, stderr="""Traceback (most recent call last):
  File \"__main__\", line 1, in <package>
TypeError: unsupported operand type(s) for +: String and Fixnum
""", stdin="""puts(\"foo\" + \"bar\")
puts(\"foo\" + 42)
""")

    def test_interactive30(self):
        self._test(stdout=">>> >>> >>> ", stderr="""Traceback (most recent call last):
  File \"__main__\", line 1, in <package>
TypeError: unsupported operand type(s) for +: String and Fixnum
Traceback (most recent call last):
  File \"__main__\", line 1, in <package>
TypeError: unsupported operand type(s) for +: String and Fixnum
""", stdin="""puts(\"foo\" + 42)
puts(\"foo\" + 42)
""")

    def test_interactive40(self):
        self._test(stdout=""">>> => 42
>>> """, stdin="""42
""")

    def test_interactive50(self):
        self._test(stdout=""">>> => 68
>>> """, stdin="""42 + 26
""")

    def test_interactive60(self):
        def test_stdout(stdout):
            self._test_regexp(r""">>> => <Object [0-9a-fA-F]+>
>>> => 42
>>> => 42
""", stdout)

        self._test(stdout=test_stdout, stdin="""o = Object.new()
o.foo = 42
o.foo
""")

    def test_SyntaxError0(self):
        self._test(stdout=">>> >>> ", stderr="""SyntaxError: file "<stdin>", line 1: Invalid syntax
""", stdin="""def def
""")

    def test_array_indexing0(self):
        self._test(stdout=""">>> => [42]
>>> => 42
>>> """, stdin="""a = [42]
a[0]
""")

# vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
