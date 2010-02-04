# -*- coding: utf-8 -*-

from testcase import TestCase

class TestPeg(TestCase):

    options = []

    def test_import0(self):
        self._test("""
import peg
enable_gc_stress()
""", "", options=["--gc-stress"])

    def test_string0(self):
        self._test("""
import peg
enable_gc_stress()
print(peg.pattern(\"foo\").match(\"foo\") != nil)
""", "true")

    def test_string3(self):
        self._test("""
import peg
enable_gc_stress()
print(peg.pattern(\"foo\").match(\"foo\").matched)
""", "foo")

    def test_string6(self):
        self._test("""
import peg
enable_gc_stress()
print(peg.pattern(\"foo\").match(\"foo\").rest)
""", "")

    def test_string10(self):
        self._test("""
import peg
enable_gc_stress()
print(peg.pattern(\"foo\").match(\"bar\"))
""", "nil")

    def test_string_serial0(self):
        self._test("""
import peg
enable_gc_stress()
print((peg.pattern(\"foo\") * peg.pattern(\"bar\")).match(\"foobar\") != nil)
""", "true")

    def test_string_serial3(self):
        self._test("""
import peg
enable_gc_stress()
print((peg.pattern(\"foo\") * peg.pattern(\"bar\")).match(\"foobar\").matched)
""", "[\"foo\", \"bar\"]")

    def test_string_serial6(self):
        self._test("""
import peg
enable_gc_stress()
print((peg.pattern(\"foo\") * peg.pattern(\"bar\")).match(\"foobar\").rest)
""", "")

    def test_string_serial10(self):
        self._test("""
import peg
enable_gc_stress()
print((peg.pattern(\"foo\") * peg.pattern(\"bar\")).match(\"quux\"))
""", "nil")

    def test_string_choice0(self):
        self._test("""
import peg
enable_gc_stress()
print((peg.pattern(\"foo\") / peg.pattern(\"bar\")).match(\"foo\") != nil)
""", "true")

    def test_string_choice3(self):
        self._test("""
import peg
enable_gc_stress()
print((peg.pattern(\"foo\") / peg.pattern(\"bar\")).match(\"foo\").matched)
""", "foo")

    def test_string_choice6(self):
        self._test("""
import peg
enable_gc_stress()
print((peg.pattern(\"foo\") / peg.pattern(\"bar\")).match(\"foo\").rest)
""", "")

    def test_string_choice10(self):
        self._test("""
import peg
enable_gc_stress()
print((peg.pattern(\"foo\") / peg.pattern(\"bar\")).match(\"quux\"))
""", "nil")

    def test_string_at_least0(self):
        self._test("""
import peg
enable_gc_stress()
print((peg.pattern(\"foo\") ^ 0).match(\"foo\") != nil)
""", "true")

    def test_string_at_least1(self):
        self._test("""
import peg
enable_gc_stress()
print((peg.pattern(\"foo\") ^ 0).match(\"foo\").matched)
""", "[\"foo\"]")

    def test_string_at_least2(self):
        self._test("""
import peg
enable_gc_stress()
print((peg.pattern(\"foo\") ^ 0).match(\"foo\").rest)
""", "")

    def test_string_at_least5(self):
        self._test("""
import peg
enable_gc_stress()
print((peg.pattern(\"foo\") ^ 0).match(\"\") != nil)
""", "true")

    def test_string_at_least6(self):
        self._test("""
import peg
enable_gc_stress()
print((peg.pattern(\"foo\") ^ 0).match(\"\").matched)
""", "[]")

    def test_string_at_least7(self):
        self._test("""
import peg
enable_gc_stress()
print((peg.pattern(\"foo\") ^ 0).match(\"\").rest)
""", "")

    def test_string_at_least10(self):
        self._test("""
import peg
enable_gc_stress()
print((peg.pattern(\"foo\") ^ 0).match(\"foofoo\") != nil)
""", "true")

    def test_string_at_least13(self):
        self._test("""
import peg
enable_gc_stress()
print((peg.pattern(\"foo\") ^ 0).match(\"foofoo\").matched)
""", "[\"foo\", \"foo\"]")

    def test_string_at_least16(self):
        self._test("""
import peg
enable_gc_stress()
print((peg.pattern(\"foo\") ^ 0).match(\"foofoo\").rest)
""", "")

    def test_string_at_least20(self):
        self._test("""
import peg
enable_gc_stress()
print((peg.pattern(\"foo\") ^ 0).match(\"baz\") != nil)
""", "true")

    def test_string_at_least21(self):
        self._test("""
import peg
enable_gc_stress()
print((peg.pattern(\"foo\") ^ 0).match(\"baz\").matched)
""", "[]")

    def test_string_at_least22(self):
        self._test("""
import peg
enable_gc_stress()
print((peg.pattern(\"foo\") ^ 0).match(\"baz\").rest)
""", "baz")

    def test_string_at_least25(self):
        self._test("""
import peg
enable_gc_stress()
print((peg.pattern(\"foo\") ^ 1).match(\"\"))
""", "nil")

    def test_string_at_least30(self):
        self._test("""
import peg
enable_gc_stress()
print((peg.pattern(\"foo\") ^ 1).match(\"foo\") != nil)
""", "true")

    def test_string_at_least33(self):
        self._test("""
import peg
enable_gc_stress()
print((peg.pattern(\"foo\") ^ 1).match(\"foo\").matched)
""", "[\"foo\"]")

    def test_string_at_least36(self):
        self._test("""
import peg
enable_gc_stress()
print((peg.pattern(\"foo\") ^ 1).match(\"foo\").rest)
""", "")

    def test_string_at_least40(self):
        self._test("""
import peg
enable_gc_stress()
print((peg.pattern(\"foo\") ^ 1).match(\"foofoo\") != nil)
""", "true")

    def test_string_at_least43(self):
        self._test("""
import peg
enable_gc_stress()
print((peg.pattern(\"foo\") ^ 1).match(\"foofoo\").matched)
""", "[\"foo\", \"foo\"]")

    def test_string_at_least46(self):
        self._test("""
import peg
enable_gc_stress()
print((peg.pattern(\"foo\") ^ 1).match(\"foofoo\").rest)
""", "")

    def test_string_at_least50(self):
        self._test("""
import peg
enable_gc_stress()
print((peg.pattern(\"foo\") ^ 1).match(\"baz\"))
""", "nil")

    def test_string_at_most0(self):
        self._test("""
import peg
enable_gc_stress()
print((peg.pattern(\"foo\") ^ (-1)).match(\"\") != nil)
""", "true")

    def test_string_at_most3(self):
        self._test("""
import peg
enable_gc_stress()
print((peg.pattern(\"foo\") ^ (-1)).match(\"\").matched)
""", "[]")

    def test_string_at_most6(self):
        self._test("""
import peg
enable_gc_stress()
print((peg.pattern(\"foo\") ^ (-1)).match(\"\").rest)
""", "")

    def test_string_at_most10(self):
        self._test("""
import peg
enable_gc_stress()
print((peg.pattern(\"foo\") ^ (-1)).match(\"foo\") != nil)
""", "true")

    def test_string_at_most13(self):
        self._test("""
import peg
enable_gc_stress()
print((peg.pattern(\"foo\") ^ (-1)).match(\"foo\").matched)
""", "[\"foo\"]")

    def test_string_at_most16(self):
        self._test("""
import peg
enable_gc_stress()
print((peg.pattern(\"foo\") ^ (-1)).match(\"foo\").rest)
""", "")

    def test_string_at_most20(self):
        self._test("""
import peg
enable_gc_stress()
print((peg.pattern(\"foo\") ^ (-1)).match(\"foofoo\") != nil)
""", "true")

    def test_string_at_most23(self):
        self._test("""
import peg
enable_gc_stress()
print((peg.pattern(\"foo\") ^ (-1)).match(\"foofoo\").matched)
""", "[\"foo\"]")

    def test_string_at_most26(self):
        self._test("""
import peg
enable_gc_stress()
print((peg.pattern(\"foo\") ^ (-1)).match(\"foofoo\").rest)
""", "foo")

    def test_string_at_most30(self):
        self._test("""
import peg
enable_gc_stress()
print((peg.pattern(\"foo\") ^ (-1)).match(\"bar\") != nil)
""", "true")

    def test_string_at_most33(self):
        self._test("""
import peg
enable_gc_stress()
print((peg.pattern(\"foo\") ^ (-1)).match(\"bar\").matched)
""", "[]")

    def test_string_at_most36(self):
        self._test("""
import peg
enable_gc_stress()
print((peg.pattern(\"foo\") ^ (-1)).match(\"bar\").rest)
""", "bar")

    def test_array_match0(self):
        self._test("""
import peg
enable_gc_stress()
print(peg.pattern(42).match([42]) != nil)
""", "true")

    def test_array_match1(self):
        self._test("""
import peg
enable_gc_stress()
print(peg.pattern(42).match([42]).matched)
""", "42")

    def test_array_match2(self):
        self._test("""
import peg
enable_gc_stress()
print(peg.pattern(42).match([42]).rest)
""", "[]")

    def test_array_match5(self):
        self._test("""
import peg
enable_gc_stress()
print(peg.pattern(42).match([26]))
""", "nil")

    def test_array_match10(self):
        self._test("""
import peg
enable_gc_stress()
print(peg.pattern(42).match([]))
""", "nil")

    def test_array_match20(self):
        self._test("""
import peg
enable_gc_stress()
print((peg.pattern(42) * peg.pattern(26)).match([42, 26]) != nil)
""", "true")

    def test_array_match23(self):
        self._test("""
import peg
enable_gc_stress()
print((peg.pattern(42) * peg.pattern(26)).match([42, 26]).matched)
""", "[42, 26]")

    def test_array_match26(self):
        self._test("""
import peg
enable_gc_stress()
print((peg.pattern(42) * peg.pattern(26)).match([42, 26]).rest)
""", "[]")

    def test_array_match30(self):
        self._test("""
import peg
enable_gc_stress()
print((peg.pattern(42) / peg.pattern(26)).match([42]) != nil)
""", "true")

    def test_array_match33(self):
        self._test("""
import peg
enable_gc_stress()
print((peg.pattern(42) / peg.pattern(26)).match([42]).matched)
""", "42")

    def test_array_match36(self):
        self._test("""
import peg
enable_gc_stress()
print((peg.pattern(42) / peg.pattern(26)).match([42]).rest)
""", "[]")

    def test_array_match40(self):
        self._test("""
import peg
enable_gc_stress()
print((peg.pattern(42) / peg.pattern(26)).match([26]) != nil)
""", "true")

    def test_array_match43(self):
        self._test("""
import peg
enable_gc_stress()
print((peg.pattern(42) / peg.pattern(26)).match([26]).matched)
""", "26")

    def test_array_match46(self):
        self._test("""
import peg
enable_gc_stress()
print((peg.pattern(42) / peg.pattern(26)).match([26]).rest)
""", "[]")

    def test_action0(self):
        self._test("""
import peg
enable_gc_stress()
pat = peg.action(peg.pattern(42)) do [matched]
  next 26
end
print(pat.match([42]).matched)
""", "26")

    def test_action10(self):
        self._test("""
import peg
enable_gc_stress()
pat = peg.action(peg.pattern(42)) do [matched]
  next 26
end
print(pat.match([42]).rest)
""", "[]")

    def test_action20(self):
        self._test("""
import peg
enable_gc_stress()
pat = peg.action(peg.pattern(42)) do [matched]
  next matched
end
print(pat.match([42]).matched)
""", "42")

    def test_action30(self):
        self._test("""
import peg
enable_gc_stress()
pat = peg.action(peg.pattern(42) * peg.pattern(26)) do [elem42, elem26]
  next elem42
end
print(pat.match([42, 26]).matched)
""", "42")

    def test_action40(self):
        self._test("""
import peg
enable_gc_stress()
pat = peg.action(peg.pattern(42) ^ 0) do [elem]
  next elem
end
print(pat.match([42, 42]).matched)
""", "[42, 42]")

    def test_action45(self):
        self._test("""
import peg
enable_gc_stress()
pat = peg.action(peg.pattern(42) ^ 0) do [elem]
  next elem
end
print(pat.match([]).matched)
""", "[]")

    def test_action50(self):
        self._test("""
import peg
enable_gc_stress()
pat = peg.action(peg.pattern(42) ^ (-1)) do [elem]
  next elem
end
print(pat.match([42, 42]).matched)
""", "[42]")

    def test_action60(self):
        self._test("""
import peg
enable_gc_stress()
pat = peg.action(peg.pattern(42) ^ (-1)) do [elem]
  next elem
end
print(pat.match([]).matched)
""", "[]")

    def test_custom_pattern0(self):
        self._test("""
import peg
enable_gc_stress()

class Foo
  def init(x)
    self.x = x
  end
end

pat = peg.pattern(Foo.new(42)) do [pat, act]
  next pat.x == act.x
end
m = pat.match([Foo.new(42)])
print(m != nil)
""", "true")

    def test_custom_pattern10(self):
        self._test("""
import peg
enable_gc_stress()

class Foo
  def init(x)
    self.x = x
  end
end

pat = peg.pattern(Foo.new(42)) do [pat, act]
  next pat.x == act.x
end
m = pat.match([Foo.new(26)])
print(m)
""", "nil")

    def test_custom_pattern20(self):
        self._test("""
import peg
enable_gc_stress()
pat = peg.pattern(42) do [pat, act]
  print(pat)
  next false
end
pat.match([26])
""", "42")

    def test_custom_pattern30(self):
        self._test("""
import peg
enable_gc_stress()
pat = peg.pattern(42) do [pat, act]
  print(act)
  next false
end
pat.match([26])
""", "26")

    def test_can_accept_empty_array0(self):
        def test_stderr(stderr):
            self._test_regexp(r"SyntaxError: loop body may accept empty array", stderr)

        self._test("""
import peg
enable_gc_stress()
(peg.pattern(\"foo\") ^ 0) ^ 0
""", stderr=test_stderr)

    def test_can_accept_empty_array20(self):
        def test_stderr(stderr):
            self._test_regexp(r"SyntaxError: loop body may accept empty array", stderr)

        self._test("""
import peg
enable_gc_stress()
(peg.pattern(\"foo\") ^ (-1)) ^ 0
""", stderr=test_stderr)

    def test_can_accept_empty_array30(self):
        def test_stderr(stderr):
            self._test_regexp(r"SyntaxError: loop body may accept empty array", stderr)

        self._test("""
import peg
enable_gc_stress()
peg.pattern(\"\") ^ 0
""", stderr=test_stderr)

# vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
