# -*- coding: utf-8 -*-

from filecmp import cmp
from os import makedirs, unlink
from os.path import abspath, isdir, join
from re import match
from shutil import rmtree
from testcase import TestCase, enumerate_tuples

class TestBuiltins(TestCase):

    def test_mod0(self):
        self._test("print(mod(42)(26))", "26")

    def test_mod10(self):
        self._test("print(mod(42)(42))", "0")

    def test_and0(self):
        self._test("print(and()(42))", "true")

    def test_and10(self):
        self._test("print(and(get(false))(42))", "false")

    def test_and20(self):
        self._test("print(and(get(true))(42))", "true")

    def test_and30(self):
        self._test("print(and(get(true), get(true))(42))", "true")

    def test_and40(self):
        self._test("print(and(get(true), get(false))(42))", "false")

    def test_and50(self):
        self._test("print(and(get(false), get(true))(42))", "false")

    def test_and60(self):
        self._test("print(and(get(true), get(true), get(true))(42))", "true")

    def test_and70(self):
        self._test("and(print + get(true), print + get(true))(42)", "4242")

    def test_or0(self):
        self._test("print(or()(42))", "false")

    def test_or10(self):
        self._test("print(or(get(false))(42))", "false")

    def test_or20(self):
        self._test("print(or(get(true))(42))", "true")

    def test_or30(self):
        self._test("print(or(get(false), get(false))(42))", "false")

    def test_or40(self):
        self._test("print(or(get(true), get(false))(42))", "true")

    def test_or50(self):
        self._test("print(or(get(false), get(true))(42))", "true")

    def test_or60(self):
        self._test("print(or(get(false), get(false), get(false))(42))", "false")

    def test_or70(self):
        self._test("or(print + get(false), print + get(false))(42)", "4242")

    def test_subscript0(self):
        self._test("print(subscript(0)([42]))", "42")

    def test_lshift0(self):
        self._test("print(lshift(1)(42))", "84")

    def test_rshift0(self):
        self._test("print(rshift(1)(42))", "21")

    def test_call_method0(self):
        src = "print(call_method(\'split, \"\\n\")(\"foo\\nbar\"))"
        self._test(src, "[\"foo\", \"bar\"]")

    for i, expr, expected in enumerate_tuples((
            ("true", "false"),
            ("false", "true"),
            ("nil", "true"),
            ("42", "false"))):
        exec("""def test_not{i}(self):
    self._test(\"print(not({expr}))\", \"{expected}\")
""".format(i=10 * i, expr=expr, expected=expected))

    def test_get0(self):
        self._test("print(get(42)())", "42")

    def test_get10(self):
        self._test("print(get(42)(26, *[26], **{ \'foo: 26 }))", "42")

    for i, f, x, y, expected in enumerate_tuples((
            ("eq", 42, 42, "true"),
            ("eq", 42, 26, "false"),
            ("neq", 42, 42, "false"),
            ("neq", 42, 26, "true"),
            ("lt", 42, 0, "true"),
            ("lt", 42, 42, "false"),
            ("lt", 26, 42, "false"),
            ("eqlt", 42, 0, "true"),
            ("eqlt", 42, 42, "true"),
            ("eqlt", 26, 42, "false"),
            ("gt", 42, 0, "false"),
            ("gt", 42, 42, "false"),
            ("gt", 26, 42, "true"),
            ("eqgt", 42, 0, "false"),
            ("eqgt", 42, 42, "true"),
            ("eqgt", 26, 42, "true"))):
        exec("""def test_{f}{i}(self):
    self._test(\"print({f}({x})({y}))\", \"{expected}\")
""".format(f=f, i=10 * i, x=x, y=y, expected=expected))

    for i, expected, arg in enumerate_tuples(((0, 0), (1, 1), (1, -1))):
        exec("""def test_abs{i}(self):
    self._test(\"print(abs({arg}))\", \"{expected}\")""".format(**locals()))

    def test_nop0(self):
        self._test("print(nop(42))", "nil")

    def test_nop10(self):
        self._test("print(nop(&42))", "nil")

    def test_max0(self):
        self._test("print(max())", "nil")

    def test_max10(self):
        self._test("print(max(42))", "42")

    def test_max20(self):
        self._test("print(max(42, 26))", "42")

    def test_max30(self):
        self._test("print(max(26, 42, 26))", "42")

    def test_min0(self):
        self._test("print(min())", "nil")

    def test_min10(self):
        self._test("print(min(42))", "42")

    def test_min20(self):
        self._test("print(min(42, 26))", "26")

    def test_min30(self):
        self._test("print(min(42, 26, 42))", "26")

    def test_run_command0(self):
        self._test("""
status, _, _ = run_command(\"/bin/echo\")
print(status)
""", "0")

    def test_run_command10(self):
        self._test("""
_, stdout, _ = run_command(\"/bin/echo\", \"foo\")
print(stdout.rtrim())
""", "foo")

    def test_run_command20(self):
        self._test("""
_, _, stderr = run_command(\"/bin/sh\", \"-c\", \"/bin/echo foo >&2\")
print(stderr.rtrim())
""", "foo")

    def test_run_command30(self):
        self._test("""
_, stdout, _ = run_command(\"/bin/cat\") do |stdin|
  stdin.write(<<EOF)
foo
EOF
end
print(stdout)
""", "foo\n")

    def test_IndexError0(self):
        def test_stderr(stderr):
            self._test_regexp(r"""Traceback \(most recent call last\):
  File "[^"]+", line 1, in <package>
IndexError: nil
""", stderr)
        self._test("raise IndexError.new()", stderr=test_stderr)

    def test_int(self):
        self._test("""
puts(42)
""", "42\n")

    def test_print0(self):
        self._test("""
print()
""", "")

    def test_print10(self):
        self._test("""
print("foo")
""", "foo")

    def test_print20(self):
        def test_stderr(stderr):
            self._test_regexp(r"""Traceback \(most recent call last\):
  File "[^"]+", line 8, in <package>
  File builtin, in print
TypeError: Foo#to_s\(\) returned non-string \(Fixnum\)
""", stderr)

        self._test("""
class Foo
  def to_s()
    return 42
  end
end

print(Foo.new())
""", stderr=test_stderr)

    def test_ARGV0(self):
        tmpfile = abspath("foo.yog")
        self._test("print(ARGV[0])", tmpfile, tmpfile=tmpfile)

    def test_ARGV1(self):
        self._test("""
print(ARGV[1])
""", "foo", yog_option=["foo"])

    def test_ARGV2(self):
        self._test("""
print(ARGV[2])
""", "bar", yog_option=["foo", "bar"])

    def test_bind0(self):
        def test_stdout(stdout):
            m = match(r"<Foo [0-9A-Za-z]+", stdout)
            assert m is not None

        self._test("""
class Foo
  def bar()
    @bind(self)
    def baz()
      print(self)
    end

    return baz
  end
end

foo = Foo.new()
bar = foo.bar()
bar()
""", stdout=test_stdout)

    def test_join_path0(self):
        self._test("""
print(join_path(\"foo\", \"bar\"))
""", join("foo", "bar"))

    def escape_special_chars(self, s):
        t = []
        for c in s:
            if c == "\\":
                t.append("\\")
            t.append(c)
        return "".join(t)

    def do_test_make_dirs(self, path, topdir):
        try:
            self._test("""
make_dirs(\"%(path)s\")
""" % { "path": self.escape_special_chars(path) })
            assert isdir(path)
        finally:
            self.delete_file(topdir)

    def delete_file(self, path):
        try:
            rmtree(path)
        except OSError:
            pass

    def test_make_dirs0(self):
        topdir = "foo"
        self.delete_file(topdir)
        self.do_test_make_dirs(topdir, topdir)

    def test_make_dirs10(self):
        topdir = "foo"
        self.delete_file(topdir)
        self.do_test_make_dirs(join(topdir, "bar"), topdir)

    def test_make_dirs20(self):
        topdir = "foo"
        self.delete_file(topdir)
        makedirs(topdir)
        self.do_test_make_dirs(join(topdir, "bar"), topdir)

    def test_mkdir0(self):
        path = "foo"
        self.delete_file(path)
        try:
            self._test("""
mkdir(\"%(path)s\")
""" % { "path": path })
            assert isdir(path)
        finally:
            rmtree(path)

    def test_copy_file0(self):
        content = "hogefugapiyo"
        src = "foo"
        fp = open(src, "wb")
        try:
            try:
                fp.write(content)
            finally:
                fp.close()

            dest = "bar"
            self._test("""
copy_file(\"%(src)s\", \"%(dest)s\")
""" % { "src": src, "dest": dest })
            try:
                fp = open(dest, "rb")
                try:
                    assert content == fp.read()
                finally:
                    fp.close()
            finally:
                unlink(dest)
        finally:
            unlink(src)

    def test_copy_file10(self):
        src = self.get_exact_path("test_copy_file10.dat")
        dest = src + "~"
        try:
            self._test("copy_file(\"{src}\", \"{dest}\")".format(**locals()))
            assert cmp(src, dest, False)
        finally:
            self.unlink(dest)

    def test_locals0(self):
        self._test("""\
def foo()
  bar = 42
  print(locals())
end

foo()""", "{ \'bar: 42 }")

    def test_locals10(self):
        self._test("""\
def foo()
  bar = 42
  baz = 26
  print(locals()[\'bar])
  print(locals()[\'baz])
end

foo()""", "4226")

    def test_id0(self):
        self._test("print(id(42))", "42")

    def test_tee0(self):
        self._test("tee(42)", "42\n")

    def test_tee10(self):
        self._test("print(tee(42))", """42
42""")

# vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
