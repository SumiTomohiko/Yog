# -*- coding: utf-8 -*-

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

    def test_escape5(self):
        self._test("""
puts(\"\\w\")
""", """w
""")

    def test_add0(self):
        self._test("""
puts(\"foo\" + \"bar\")
""", """foobar
""")

    def test_add3(self):
        self._test("""
puts("foo" + "bar")
""", """foobar
""")

    def test_add5(self):
        self._test("""
puts("foo" + "bar" + "baz")
""", """foobarbaz
""")

    def test_add10(self):
        def test_stderr(stderr):
            self._test_regexp(r"""Traceback \(most recent call last\):
  File "[^"]+", line 2, in <package>
TypeError: unsupported operand type\(s\) for \+: String and Fixnum
""", stderr)

        self._test("""
puts("foo" + 42)
""", stderr=test_stderr)

    def test_add20(self):
        def test_stderr(stderr):
            self._test_regexp(r"""Traceback \(most recent call last\):
  File "[^"]+", line 2, in <package>
TypeError: unsupported operand type\(s\) for \+: String and Bool
""", stderr)

        self._test("""
puts("foo" + true)
""", stderr=test_stderr)

    def test_add30(self):
        def test_stderr(stderr):
            self._test_regexp(r"""Traceback \(most recent call last\):
  File "[^"]+", line 2, in <package>
TypeError: unsupported operand type\(s\) for \+: String and Symbol
""", stderr)

        self._test("""
puts("foo" + 'bar)
""", stderr=test_stderr)

    def test_add40(self):
        def test_stderr(stderr):
            self._test_regexp(r"""Traceback \(most recent call last\):
  File "[^"]+", line 2, in <package>
TypeError: unsupported operand type\(s\) for \+: String and Nil
""", stderr)

        self._test("""
puts("foo" + nil)
""", stderr=test_stderr)

    def test_add50(self):
        def test_stderr(stderr):
            self._test_regexp(r"""Traceback \(most recent call last\):
  File "[^"]+", line 2, in <package>
TypeError: unsupported operand type\(s\) for \+: String and Float
""", stderr)

        self._test("""
puts("foo" + 3.1415926535)
""", stderr=test_stderr)

    def test_add60(self):
        self._test(u"""
print((\"九\" + \"頭龍\")[0])
""", u"九")

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

    def test_lshift30(self):
        self._test("""
s = \"\"
s << \"foo\"
s << \"bar\"
puts(s)
""", """foobar
""")

    def test_lshift40(self):
        self._test("""
s = \"foo\"
s << \"bar\"
s << \"baz\"
puts(s)
""", """foobarbaz
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
        self._test(u"""
\"日本語\".each_char() do [c]
  puts(c)
end""", u"""日
本
語
""")

    def test_each_char3(self):
        self._test("""
\"\".each_char() do [c]
  print(42)
end
""", "")

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

    def test_each_line3(self):
        self._test("""
\"\".each_line() do [line]
  print(42)
end
""", "")

    def test_assign_subscript1(self):
        self._test("""
s = \"foo\"
s[0] = \"b\"
puts(s)
""", """boo
""")

    def test_assign_subscript2(self):
        self._test(u"""
s = \"foo\"
s[0] = \"燦\"
puts(s)
""", u"""燦oo
""")

    def test_assign_subscript2(self):
        self._test(u"""
s = \"燦oo\"
s[0] = \"f\"
puts(s)
""", """foo
""")

    def test_assign_subscript10(self):
        self._test("""
s = \"foo\"
s[-1] = \"b\"
print(s)
""", "fob")

    def test_subscript1(self):
        self._test("""
s = \"foo\"
puts(s[0])
""", """f
""")

    def test_subscript2(self):
        self._test(u"""
s = \"燦oo\"
puts(s[0])
""", u"""燦
""")

    def test_subscript3(self):
        self._test(u"""
s = \"f燦o\"
puts(s[1])
""", u"""燦
""")

    def test_subscript10(self):
        self._test("""
s = \"bar\"
print(s[-1])
""", "r")

    def test_subscript_error0(self):
        def test_stderr(stderr):
            self._test_regexp(r"""Traceback \(most recent call last\):
  File "[^"]+", line 3, in <package>
IndexError: string index out of range
""", stderr)

        self._test("""
s = \"\"
puts(s[0])
""", stderr=test_stderr)

    def test_subscript_error10(self):
        def test_stderr(stderr):
            self._test_regexp(r"""Traceback \(most recent call last\):
  File "[^"]+", line 3, in <package>
IndexError: string index out of range
""", stderr)

        self._test("""
s = \"\"
puts(s[1])
""", stderr=test_stderr)

    def test_subscript_error20(self):
        def test_stderr(stderr):
            self._test_regexp(r"""Traceback \(most recent call last\):
  File "[^"]+", line 3, in <package>
IndexError: string index out of range
""", stderr)

        self._test("""
s = \"\"
puts(s[-1])
""", stderr=test_stderr)

    def test_subscript_error30(self):
        def test_stderr(stderr):
            self._test_regexp(r"""Traceback \(most recent call last\):
  File "[^"]+", line 3, in <package>
TypeError: String index must be Fixnum
""", stderr)

        self._test("""
s = \"\"
puts(s[\"\"])
""", stderr=test_stderr)

    def test_assign_subscript_error0(self):
        def test_stderr(stderr):
            self._test_regexp(r"""Traceback \(most recent call last\):
  File "[^"]+", line 3, in <package>
  File builtin, in String#\[\]=
IndexError: string index out of range
""", stderr)

        self._test("""
s = \"\"
s[0] = \"\"
""", stderr=test_stderr)

    def test_assign_subscript_error10(self):
        def test_stderr(stderr):
            self._test_regexp(r"""Traceback \(most recent call last\):
  File "[^"]+", line 3, in <package>
  File builtin, in String#\[\]=
IndexError: string index out of range
""", stderr)

        self._test("""
s = \"\"
s[1] = \"\"
""", stderr=test_stderr)

    def test_assign_subscript_error20(self):
        def test_stderr(stderr):
            self._test_regexp(r"""Traceback \(most recent call last\):
  File "[^"]+", line 3, in <package>
  File builtin, in String#\[\]=
IndexError: string index out of range
""", stderr)

        self._test("""
s = \"\"
s[-1] = \"\"
""", stderr=test_stderr)

    def test_assign_subscript_error30(self):
        def test_stderr(stderr):
            self._test_regexp(r"""Traceback \(most recent call last\):
  File "[^"]+", line 3, in <package>
  File builtin, in String#\[\]=
TypeError: string index must be Fixnum
""", stderr)

        self._test("""
s = \"\"
s[\"\"] = \"\"
""", stderr=test_stderr)

    def test_multiply0(self):
        def test_stderr(stderr):
            self._test_regexp(r"""Traceback \(most recent call last\):
  File "[^"]+", line 2, in <package>
TypeError: Can't multiply string by non-Fixnum of type String
""", stderr)

        self._test("""
puts("foo" * "bar")
""", stderr=test_stderr)

    def test_multiply10(self):
        self._test("""
puts("foo" * 2)
""", """foofoo
""")

    def test_multiply20(self):
        def test_stderr(stderr):
            self._test_regexp(r"""Traceback \(most recent call last\):
  File "[^"]+", line 2, in <package>
TypeError: Can't multiply string by non-Fixnum of type Bool
""", stderr)

        self._test("""
puts("foo" * true)
""", stderr=test_stderr)

    def test_multiply30(self):
        def test_stderr(stderr):
            self._test_regexp(r"""Traceback \(most recent call last\):
  File "[^"]+", line 2, in <package>
TypeError: Can't multiply string by non-Fixnum of type Symbol
""", stderr)

        self._test("""
puts("foo" * 'bar)
""", stderr=test_stderr)

    def test_multiply40(self):
        def test_stderr(stderr):
            self._test_regexp(r"""Traceback \(most recent call last\):
  File "[^"]+", line 2, in <package>
TypeError: Can't multiply string by non-Fixnum of type Nil
""", stderr)

        self._test("""
puts("foo" * nil)
""", stderr=test_stderr)

    def test_multiply50(self):
        def test_stderr(stderr):
            self._test_regexp(r"""Traceback \(most recent call last\):
  File "[^"]+", line 2, in <package>
TypeError: Can't multiply string by non-Fixnum of type Float
""", stderr)

        self._test("""
puts("foo" * 3.1415926535)
""", stderr=test_stderr)

    def test_multiply60(self):
        def test_stderr(stderr):
            self._test_regexp(r"""Traceback \(most recent call last\):
  File "[^"]+", line 2, in <package>
ArgumentError: Negative argument
""", stderr)

        self._test("""
puts("foo" * (-1))
""", stderr=test_stderr)

    def test_gsub0(self):
        self._test("""
print("foo".gsub("o", "bar"))
""", "fbarbar")

    def test_format0(self):
        self._test("""
print(\"{0}\".format(42))
""", "42")

    def test_format10(self):
        self._test("""
print(\"{0} and {1}\".format(42, 26))
""", "42 and 26")

    def test_format20(self):
        self._test("""
print(\"{0} and {0}\".format(42))
""", "42 and 42")

    def test_format30(self):
        self._test("""
print(\"{{\".format())
""", "{")

    def test_format40(self):
        self._test("""
print(\"}}\".format())
""", "}")

    def test_find0(self):
        self._test("""
print(\"foo\".find(\"o\"))
""", "1")

    def test_find10(self):
        self._test("""
print(\"foo\".find(\"bar\"))
""", "-1")

    def test_find20(self):
        self._test("""
print(\"foo\".find(\"barbazquux\"))
""", "-1")

    def test_find30(self):
        self._test("""
print(\"foo\".find(\"o\", 0))
""", "1")

    def test_find40(self):
        self._test("""
print(\"foo\".find(\"o\", 1))
""", "1")

    def test_find50(self):
        self._test("""
print(\"foo\".find(\"o\", 2))
""", "2")

    def test_find60(self):
        self._test("""
print(\"foo\".find(\"o\", 3))
""", "-1")

    def test_find70(self):
        self._test("""
print(\"foo\".find(\"o\", -1))
""", "2")

    def test_find80(self):
        self._test("""
print(\"foo\".find(\"o\", -4))
""", "1")

    def test_ltrim0(self):
        self._test("""
print(\" foo\".ltrim())
""", "foo")

    def test_ltrim10(self):
        self._test("""
print(\"foo\".ltrim())
""", "foo")

    def test_ltrim20(self):
        self._test("""
print(\"   \".ltrim())
""", "")

    def test_rtrim0(self):
        self._test("""
print(\"foo \".rtrim())
""", "foo")

    def test_rtrim10(self):
        self._test("""
print(\"foo\".rtrim())
""", "foo")

    def test_rtrim20(self):
        self._test("""
print(\"   \".rtrim())
""", "")

    def test_trim0(self):
        self._test("""
print(\" foo \".trim())
""", "foo")

    def test_trim10(self):
        self._test("""
print(\"foo\".trim())
""", "foo")

    def test_dup0(self):
        self._test("""
print(\"foo\".dup())
""", "foo")

    def test_slice0(self):
        self._test("""
print(\"foo\".slice(0))
""", "foo")

    def test_slice10(self):
        self._test("""
print(\"foo\".slice(1))
""", "oo")

    def test_slice20(self):
        self._test("""
print(\"foo\".slice(-1))
""", "o")

    def test_slice30(self):
        self._test("""
print(\"foo\".slice(3))
""", "")

    def test_slice40(self):
        self._test("""
print(\"foo\".slice(0, 0))
""", "")

    def test_slice50(self):
        self._test("""
print(\"foo\".slice(0, 1))
""", "f")

    def test_slice60(self):
        self._test("""
print(\"foo\".slice(0, 3))
""", "foo")

    def test_slice70(self):
        self._test("""
print(\"foo\".slice(0, 4))
""", "foo")

    def test_slice80(self):
        self._test("""
print(\"foo\".slice(0, -1))
""", "")

    def test_starts_with0(self):
        self._test("""
print(\"foo\".starts_with?(\"f\"))
""", "true")

    def test_starts_with10(self):
        self._test("""
print(\"foo\".starts_with?(\"foo\"))
""", "true")

    def test_starts_with15(self):
        self._test("""
print(\"foo\".starts_with?(\"fooo\"))
""", "false")

    def test_starts_with20(self):
        self._test("""
print(\"foo\".starts_with?(\"bar\"))
""", "false")

    def test_inspect0(self):
        self._test("""
print(\"foo\".inspect())
""", "\"foo\"")

    def test_inspect10(self):
        self._test("""
print(\"\\n\".inspect())
""", "\"\\n\"")

    def test_inspect20(self):
        self._test("""
print(\"\\t\".inspect())
""", "\"\\t\"")

    def test_inspect30(self):
        self._test("""
print(\"\\\\\".inspect())
""", "\"\\\\\"")

    def test_inspect40(self):
        self._test("""
print(\"\".inspect())
""", "\"\"")

    def test_split0(self):
        self._test("""
print(\"foo\".split(\"o\"))
""", "[\"f\", \"\", \"\"]")

    def test_split10(self):
        self._test("""
print(\"foo\".split(\"\"))
""", "[\"f\", \"o\", \"o\"]")

    def test_split20(self):
        self._test("""
print(\"foo\\nbar\".split(\"\\n\"))
""", "[\"foo\", \"bar\"]")

    def test_split30(self):
        self._test("""
print(\"foo bar\".split())
""", "[\"foo\", \"bar\"]")

    def test_split40(self):
        self._test("""
print(\"foo\\nbar\".split())
""", "[\"foo\", \"bar\"]")

    def test_split50(self):
        self._test("""
print(\"foo\\tbar\".split())
""", "[\"foo\", \"bar\"]")

    def test_split60(self):
        self._test("""
print(\"foo\".split(//))
""", "[\"f\", \"o\", \"o\"]")

    def test_get0(self):
        self._test("""
print(\"\".get(0, \"foo\"))
""", "foo")

    def test_get10(self):
        self._test("""
print(\"foo\".get(0, \"bar\"))
""", "f")

    def test_get20(self):
        self._test("""
print(\"foo\".get(0))
""", "f")

    def test_get30(self):
        def test_stderr(stderr):
            self._test_regexp(r"""Traceback \(most recent call last\):
  File "[^"]+", line 2, in <package>
  File builtin, in String#get
IndexError: string index out of range
""", stderr)

        self._test("""
\"\".get(0)
""", stderr=test_stderr)

    def test_rfind0(self):
        self._test("""
print(\"foo\".rfind(\"o\"))
""", "2")

    def test_rfind10(self):
        self._test("""
print(\"foo\".rfind(\"bar\"))
""", "-1")

    def test_rfind20(self):
        self._test("""
print(\"foo\".rfind(\"barbazquux\"))
""", "-1")

    def test_rfind30(self):
        self._test("""
print(\"foo\".rfind(\"o\", 0))
""", "-1")

    def test_rfind40(self):
        self._test("""
print(\"foo\".rfind(\"o\", 1))
""", "1")

    def test_rfind50(self):
        self._test("""
print(\"foo\".rfind(\"o\", 2))
""", "2")

    def test_rfind60(self):
        self._test("""
print(\"foo\".rfind(\"o\", 3))
""", "2")

    def test_rfind70(self):
        self._test("""
print(\"foo\".rfind(\"o\", -1))
""", "2")

    def test_rfind80(self):
        self._test("""
print(\"foo\".rfind(\"o\", -4))
""", "-1")

# vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
