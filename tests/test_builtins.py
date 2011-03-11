# -*- coding: utf-8 -*-

from re import match
from os import makedirs, unlink
from os.path import isdir, join
from shutil import rmtree
from testcase import TestCase

class TestBuiltins(TestCase):

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
_, stdout, _ = run_command(\"/bin/cat\") do [stdin]
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
        self._test("""
print(ARGV[0])
""", "foo.yg", tmpfile="foo.yg")

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

# vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
