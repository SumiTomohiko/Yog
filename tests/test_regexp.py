# -*- coding: utf-8 -*-

from testcase import TestCase

class TestRegexp(TestCase):

    def test_literal10(self):
        self._test("""
s = \"\"
s =~ //""")

    def test_literal20(self):
        self._test("""
s = \"\"
s =~ //i""")

    def test_literal30(self):
        self._test("""
s = \"\"
s =~ /foo/i""")

    def test_search_op_expr10(self):
        self._test("""
s = \"foo\"
if s =~ /foo/
  puts(42)
end""", """42
""")

    def test_search_op_expr11(self):
        self._test("""
print((\"\" =~ /\\A/) != nil)
""", "true")

    def test_search_op_expr15(self):
        self._test("""
s = \"foobarbaz\"
if s =~ /bar/
  puts(42)
end""", """42
""")

    def test_search_op_expr20(self):
        self._test("""
s = \"foo\"
if s =~ /bar/
else
  puts(42)
end""", """42
""")

    def test_search_op_ignore_case10(self):
        self._test("""
s = \"foo\"
if s =~ /FOO/i
  puts(42)
end""", """42
""")

    def test_search_op_group05(self):
        self._test("""
m = \"foo\" =~ /foo/
puts(m.group())
""", """foo
""")

    def test_search_op_group10(self):
        self._test("""
m = \"foo\" =~ /foo/
puts(m.group(0))
""", """foo
""")

    def test_search_op_group15(self):
        self._test("""
m = \"foobarbaz\" =~ /bar/
puts(m.group())
""", """bar
""")

    def test_search_op_group20(self):
        self._test("""
m = \"foobarbaz\" =~ /bar/
puts(m.group(0))
""", """bar
""")

    def test_search_op_group30(self):
        self._test("""
m = \"foobarbazquux\" =~ /foo(bar)baz/
puts(m.group(1))
""", """bar
""")

    def test_search_op_group40(self):
        self._test("""
m = \"foobarbazquux\" =~ /foo(?<name>bar)baz/
puts(m.group(\"name\"))
""", """bar
""")

    def test_search_op_group50(self):
        def test_stderr(stderr):
            self._test_regexp(r"""Traceback \(most recent call last\):
  File "[^"]+", line 3, in <package>
  File builtin, in Match#group
IndexError: No such group: 42
""", stderr)

        self._test("""
m = \"foo\" =~ /foo/
puts(m.group(42))
""", stderr=test_stderr)

    def test_search_op_group60(self):
        def test_stderr(stderr):
            self._test_regexp(r"""Traceback \(most recent call last\):
  File "[^"]+", line 2, in <package>
  File builtin, in Match#group
TypeError: group must be a Fixnum, String or nil, not Array
""", stderr)

        self._test("""
(\"foo\" =~ /foo/).group([])
""", stderr=test_stderr)

    def test_search_op_group70(self):
        self._test("print(/(foo)?/.match(\"bar\").group(1))", "nil")

    def test_search_op_start05(self):
        self._test("""
m = \"foo\" =~ /foo/
puts(m.start())
""", """0
""")

    def test_search_op_start10(self):
        self._test("""
m = \"foo\" =~ /foo/
puts(m.start(0))
""", """0
""")

    def test_search_op_start15(self):
        self._test("""
m = \"foobarbaz\" =~ /bar/
puts(m.start())
""", """3
""")

    def test_search_op_start20(self):
        self._test("""
m = \"foobarbaz\" =~ /bar/
puts(m.start(0))
""", """3
""")

    def test_search_op_start30(self):
        self._test("""
m = \"foobarbazquux\" =~ /foo(bar)baz/
puts(m.start(1))
""", """3
""")

    def test_search_op_start40(self):
        self._test("""
m = \"foobarbazquux\" =~ /foo(?<name>bar)baz/
puts(m.start(\"name\"))
""", """3
""")

    def test_search_op_start45(self):
        self._test("""
m = (\"foo\" =~ /\\A/)
print(m.start(0))
""", "0")

    def test_search_op_start46(self):
        self._test("""
m = (\"foo\" =~ /\\Z/)
print(m.start(0))
""", "3")

    def test_search_op_start50(self):
        def test_stderr(stderr):
            self._test_regexp(r"""Traceback \(most recent call last\):
  File "[^"]+", line 3, in <package>
  File builtin, in Match#start
IndexError: No such group: 42
""", stderr)

        self._test("""
m = \"foo\" =~ /foo/
puts(m.start(42))
""", stderr=test_stderr)

    def test_search_op_start60(self):
        self._test("print(/(foo)?/.match(\"bar\").start(1))", "nil")

    def test_search_op_end05(self):
        self._test("""
m = \"foo\" =~ /foo/
puts(m.end())
""", """3
""")

    def test_search_op_end10(self):
        self._test("""
m = \"foo\" =~ /foo/
puts(m.end(0))
""", """3
""")

    def test_search_op_end15(self):
        self._test("""
m = \"foobarbaz\" =~ /bar/
puts(m.end())
""", """6
""")

    def test_search_op_end20(self):
        self._test("""
m = \"foobarbaz\" =~ /bar/
puts(m.end(0))
""", """6
""")

    def test_search_op_end30(self):
        self._test("""
m = \"foobarbazquux\" =~ /foo(bar)baz/
puts(m.end(1))
""", """6
""")

    def test_search_op_end40(self):
        self._test("""
m = \"foobarbazquux\" =~ /foo(?<name>bar)baz/
puts(m.end(\"name\"))
""", """6
""")

    def test_search_op_end45(self):
        self._test("""
m = (\"foo\" =~ /\\A/)
print(m.end(0))
""", "0")

    def test_search_op_end46(self):
        self._test("""
m = (\"foo\" =~ /\\Z/)
print(m.end(0))
""", "3")

    def test_search_op_end50(self):
        def test_stderr(stderr):
            self._test_regexp(r"""Traceback \(most recent call last\):
  File "[^"]+", line 3, in <package>
  File builtin, in Match#end
IndexError: No such group: 42
""", stderr)

        self._test("""
m = \"foo\" =~ /foo/
puts(m.end(42))
""", stderr=test_stderr)

    def test_search_op_end60(self):
        self._test("print(/(foo)?/.match(\"bar\").end(1))", "nil")

    def test_match0(self):
        self._test("print(/foo/.match(\"foo\") != nil)", "true")

    def test_match10(self):
        self._test("print(/bar/.match(\"foobar\") != nil)", "false")

    def test_match20(self):
        self._test("print(/bar/.match(\"foobar\", 3) != nil)", "true")

    def test_match30(self):
        self._test("print(/foo/.match(\"foobar\", 3) != nil)", "false")

    def test_search0(self):
        self._test("print(/foo/.search(\"foo\") != nil)", "true")

    def test_search10(self):
        self._test("print(/bar/.search(\"foobar\") != nil)", "true")

    def test_search20(self):
        self._test("print(/bar/.search(\"foobar\", 3) != nil)", "true")

    def test_search30(self):
        self._test("print(/foo/.search(\"foobar\", 3) != nil)", "false")

# vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
