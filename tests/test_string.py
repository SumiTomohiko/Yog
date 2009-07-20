# -*- coding: utf-8 -*-

from re import match

from testcase import TestCase

class TestString(TestCase):

    def test_literal1(self):
        self._test("""
puts(\"foo\")
""", """foo
""")

    def test_literal2(self):
        self._test("""
puts(\"\")
""", """
""")

    def test_literal3(self):
        self._test("""
puts(\"'\")
""", """'
""")

    def test_literal4(self):
        self._test("""
puts('foo')
""", """foo
""")

    def test_literal5(self):
        self._test("""
puts('')
""", """
""")

    def test_literal6(self):
        self._test("""
puts('\"')
""", """\"
""")

    def test_literal7(self):
        self._test("""
puts(\"\\n\")
""", """

""")

    def test_escape1(self):
        self._test("""
puts(\"\\\"\")
""", """\"
""")

    def test_escape2(self):
        self._test("""
puts(\"\\\\\")""", """\\
""")

    def test_escape3(self):
        self._test("""
puts('\\'')
""", """'
""")

    def test_escape4(self):
        self._test("""
puts('\\\\')
""", """\\
""")

    def test_escape5(self):
        self._test("""
puts(\"\\w\")
""", """w
""")

    def test_add(self):
        self._test("""
puts(\"foo\" + \"bar\")
""", """foobar
""")

    def test_lshift1(self):
        self._test("""
s = \"foo\"
puts(s << \"bar\")
""", """foobar
""")

    def test_lshift2(self):
        self._test("""
s = \"foo\"
s << \"bar\"
puts(s)
""", """foobar
""")

    def test_each_char1(self):
        self._test("""
\"foo\".each_char() do [c]
  puts(c)
end""", """f
o
o
""")

    def test_each_char2(self):
        self._test("""
\"日本語\".each_char() do [c]
  puts(c)
end""", """日
本
語
""")

    def test_each_byte1(self):
        self._test("""
\"foo\".each_byte() do [b]
  puts(b)
end""", """102
111
111
""")

    def test_each_byte2(self):
        self._test("""
\"日本語\".each_byte() do [b]
  puts(b)
end""", """230
151
165
230
156
172
232
170
158
""")

    def test_each_line1(self):
        self._test("""
\"foo\".each_line() do [l]
  puts(l)
end""", """foo
""")

    def test_each_line2(self):
        self._test("""
\"foo\\nbar\".each_line() do [l]
  puts(l)
end""", """foo
bar
""")

    def test_assign_subscript1(self):
        self._test("""
s = \"foo\"
s[0] = \"b\"
puts(s)
""", """boo
""")

    def test_assign_subscript2(self):
        self._test("""
s = \"foo\"
s[0] = \"燦\"
puts(s)
""", """燦oo
""")

    def test_assign_subscript2(self):
        self._test("""
s = \"燦oo\"
s[0] = \"f\"
puts(s)
""", """foo
""")

    def test_subscript1(self):
        self._test("""
s = \"foo\"
puts(s[0])
""", """f
""")

    def test_subscript2(self):
        self._test("""
s = \"燦oo\"
puts(s[0])
""", """燦
""")

    def test_subscript3(self):
        self._test("""
s = \"f燦o\"
puts(s[1])
""", """燦
""")

    def test_subscript_error1(self):
        def test_stderr(stderr):
            m = match(r"""Traceback \(most recent call last\):
  File "[^"]+", line 3, in <module>
  File builtin, in String#\[\]
IndexError: string index out of range
""", stderr)
            assert m is not None

        self._test("""
s = \"\"
puts(s[0])
""", stderr=test_stderr)

    def test_subscript_error2(self):
        def test_stderr(stderr):
            m = match(r"""Traceback \(most recent call last\):
  File "[^"]+", line 3, in <module>
  File builtin, in String#\[\]
IndexError: string index out of range
""", stderr)
            assert m is not None

        self._test("""
s = \"\"
puts(s[1])
""", stderr=test_stderr)

    def test_subscript_error3(self):
        def test_stderr(stderr):
            m = match(r"""Traceback \(most recent call last\):
  File "[^"]+", line 3, in <module>
  File builtin, in String#\[\]
TypeError: string index must be integer
""", stderr)
            assert m is not None

        self._test("""
s = \"\"
puts(s[\"\"])
""", stderr=test_stderr)

    def test_assign_subscript_error1(self):
        def test_stderr(stderr):
            m = match(r"""Traceback \(most recent call last\):
  File "[^"]+", line 3, in <module>
  File builtin, in String#\[\]=
TypeError: string index must be integer
""", stderr)
            assert m is not None

        self._test("""
s = \"\"
s[\"foo\"] = \"bar\"""", stderr=test_stderr)

    def test_add0(self):
        self._test("""
puts("foo" + "bar")
""", """foobar
""")

    def test_add10(self):
        def test_stderr(stderr):
            m = match(r"""Traceback \(most recent call last\):
  File "[^"]+", line 2, in <module>
  File builtin, in String#\+
TypeError: can't convert 'Int' object to string implicitly
""", stderr)
            assert m is not None

        self._test("""
puts("foo" + 42)
""", stderr=test_stderr)

    def test_add20(self):
        def test_stderr(stderr):
            m = match(r"""Traceback \(most recent call last\):
  File "[^"]+", line 2, in <module>
  File builtin, in String#\+
TypeError: can't convert 'Bool' object to string implicitly
""", stderr)
            assert m is not None

        self._test("""
puts("foo" + true)
""", stderr=test_stderr)

    def test_add30(self):
        def test_stderr(stderr):
            m = match(r"""Traceback \(most recent call last\):
  File "[^"]+", line 2, in <module>
  File builtin, in String#\+
TypeError: can't convert 'Symbol' object to string implicitly
""", stderr)
            assert m is not None

        self._test("""
puts("foo" + :bar)
""", stderr=test_stderr)

    def test_add40(self):
        def test_stderr(stderr):
            m = match(r"""Traceback \(most recent call last\):
  File "[^"]+", line 2, in <module>
  File builtin, in String#\+
TypeError: can't convert 'Nil' object to string implicitly
""", stderr)
            assert m is not None

        self._test("""
puts("foo" + nil)
""", stderr=test_stderr)

    def test_add50(self):
        def test_stderr(stderr):
            m = match(r"""Traceback \(most recent call last\):
  File "[^"]+", line 2, in <module>
  File builtin, in String#\+
TypeError: can't convert 'Float' object to string implicitly
""", stderr)
            assert m is not None

        self._test("""
puts("foo" + 3.1415926535)
""", stderr=test_stderr)

# vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
