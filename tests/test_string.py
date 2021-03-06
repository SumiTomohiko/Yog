# -*- coding: utf-8 -*-

from os.path import join
from testcase import TestCase, enumerate_tuples

class TestString(TestCase):

    def test_empty0(self):
        self._test("print(\"\".empty?)", "true")

    def test_empty10(self):
        self._test("print(\"x\".empty?)", "false")

    def test_compare0(self):
        self._test("print(\"g\" < \"foo\")", "false")

    def test_compare10(self):
        self._test("print(\"foo\" < \"g\")", "true")

    def test_compare20(self):
        self._test("print(\"foo\" < \"goo\")", "true")

    def test_compare30(self):
        self._test("print(\"goo\" < \"foo\")", "false")

    def test_compare40(self):
        self._test("print(\"foo\" < \"foo\")", "false")

    def test_compare50(self):
        self._test("print(\"fooo\" < \"foo\")", "false")

    def test_compare60(self):
        self._test("print(\"foo\" < \"fooo\")", "true")

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

    def test_each1(self):
        self._test("""
\"foo\".each() do |c|
  puts(c)
end""", """f
o
o
""")

    def test_each2(self):
        self._test(u"""
\"日本語\".each() do |c|
  puts(c)
end""", u"""日
本
語
""")

    def test_each3(self):
        self._test("""
\"\".each() do |c|
  print(42)
end
""", "")

    def test_each_line1(self):
        self._test("""
\"foo\".each_line() do |l|
  puts(l)
end""", """foo
""")

    def test_each_line2(self):
        self._test("""
\"foo\\nbar\".each_line() do |l|
  puts(l)
end""", """foo
bar
""")

    def test_each_line3(self):
        self._test("""
\"\".each_line() do |line|
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
IndexError: String index out of range
""", stderr)

        self._test("""
s = \"\"
puts(s[0])
""", stderr=test_stderr)

    def test_subscript_error10(self):
        def test_stderr(stderr):
            self._test_regexp(r"""Traceback \(most recent call last\):
  File "[^"]+", line 3, in <package>
IndexError: String index out of range
""", stderr)

        self._test("""
s = \"\"
puts(s[1])
""", stderr=test_stderr)

    def test_subscript_error20(self):
        def test_stderr(stderr):
            self._test_regexp(r"""Traceback \(most recent call last\):
  File "[^"]+", line 3, in <package>
IndexError: String index out of range
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
IndexError: String index out of range
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
IndexError: String index out of range
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
IndexError: String index out of range
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
TypeError: String index must be Fixnum
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

    def test_format_spec_width0(self):
        self._test("print(\"{0:1}\".format(42))", "42")

    def test_format_spec_width10(self):
        self._test("print(\"{0:1}\".format(0))", "0")

    def test_format_spec_width20(self):
        self._test("print(\"{0:2}\".format(128))", "128")

    def test_format_spec_width30(self):
        self._test("print(\"{0:2}\".format(42))", "42")

    def test_format_spec_width40(self):
        self._test("print(\"{0:2}\".format(0))", " 0")

    def test_format_spec_width50(self):
        self._test("print(\"{0:02}\".format(128))", "128")

    def test_format_spec_width60(self):
        self._test("print(\"{0:02}\".format(42))", "42")

    def test_format_spec_width70(self):
        self._test("print(\"{0:02}\".format(0))", "00")

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

    def test_inspect50(self):
        self._test("print(\"\\\"\".inspect())", "\"\\\"\"")

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

    def test_split70(self):
        self._test("print(\"foo bar\".split(max: 0))", "[\"foo bar\"]")

    def test_split80(self):
        self._test("print(\"foo bar\".split(max: 1))", "[\"foo\", \"bar\"]")

    def test_split90(self):
        self._test("print(\"foo bar\".split(max: 2))", "[\"foo\", \"bar\"]")

    def test_split100(self):
        src = "print(\"foo bar baz\".split(max: 1))"
        self._test(src, "[\"foo\", \"bar baz\"]")

    def test_split110(self):
        src = "print(\"foo+bar\".split(\"+\", 0))"
        self._test(src, "[\"foo+bar\"]")

    def test_split120(self):
        src = "print(\"foo+bar\".split(\"+\", 1))"
        self._test(src, "[\"foo\", \"bar\"]")

    def test_split130(self):
        src = "print(\"foo+bar\".split(\"+\", 2))"
        self._test(src, "[\"foo\", \"bar\"]")

    def test_split140(self):
        src = "print(\"foo+bar+baz\".split(\"+\", 1))"
        self._test(src, "[\"foo\", \"bar+baz\"]")

    def test_get0(self):
        self._test("print(\"\".get(0))", "nil")

    def test_get5(self):
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

    def test_to_sym0(self):
        self._test("print(\"foo\".to_sym().inspect())", "\'foo")

    def test_to_path0(self):
        path = join("foo", "bar")
        self._test("print(\"%(path)s\".to_path().dirname)" % locals(), "foo")

    def test_divide0(self):
        self._test("print((\"foo\" / \"bar\").dirname)", "foo")

    def test_as_bin0(self):
        self._test("print(\"101010\".as_bin())", "42")

    def test_as_oct0(self):
        self._test("print(\"42\".as_oct())", "34")

    def test_as_hex0(self):
        self._test("print(\"42\".as_hex())", "66")

    def test_as_dec0(self):
        self._test("print(\"42\".as_dec())", "42")

    def test_to_i0(self):
        self._test("print(\"42\".to_i())", "42")

    def test_to_i10(self):
        self._test("print(\"42\".to_i(16))", "66")

    def test_to_i20(self):
        self._test("print(\"42\".to_i(10))", "42")

    def test_to_i30(self):
        self._test("print(\"42\".to_i(8))", "34")

    def test_to_i40(self):
        self._test("print(\"101010\".to_i(2))", "42")

    for i, s, expected in enumerate_tuples((
            ("", []),
            ("foo", [0x66, 0x6f, 0x6f]))):
        for meth, additional in (("to_cstr", [0]), ("to_bin", [])):
            exec("""def test_{meth}{i}(self):
    self._test(\"print(\\\"{s}\\\".{meth}(ENCODINGS[\\\"ascii\\\"]).to_a())\", \"{expected}\")
""".format(i=10 * i, s=s, meth=meth, expected=expected + additional))

    for i, s, expected in enumerate_tuples((
            ("", ""),
            ("foo", "FOO"),
            ("Foo", "FOO"),
            ("foo bar", "FOO BAR"))):
        exec("""def test_to_upper{i}(self):
    self._test(\"print(\\\"{s}\\\".to_upper())\", \"{expected}\")
""".format(i=10 * i, s=s, expected=expected))

    for i, s, expected in enumerate_tuples((
            ("", ""),
            ("FOO", "foo"),
            ("fOO", "foo"),
            ("FOO BAR", "foo bar"))):
        exec("""def test_to_lower{i}(self):
    self._test(\"print(\\\"{s}\\\".to_lower())\", \"{expected}\")
""".format(i=10 * i, s=s, expected=expected))

    for i, s, expected in enumerate_tuples((
            ("", ""),
            ("foo", "Foo"),
            ("foo bar", "FooBar"),
            ("foo_bar", "FooBar"))):
        exec("""def test_to_upper_camel_case{i}(self):
    self._test(\"print(\\\"{s}\\\".to_upper_camel_case())\", \"{expected}\")
""".format(i=10 * i, s=s, expected=expected))

        if expected == "":
            lower_expected = ""
        else:
            lower_expected = expected[0].lower() + expected[1:]
        exec("""def test_to_lower_camel_case{i}(self):
    self._test(\"print(\\\"{s}\\\".to_lower_camel_case())\", \"{expected}\")
""".format(i=10 * i, s=s, expected=lower_expected))

# vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
