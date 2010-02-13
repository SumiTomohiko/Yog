# -*- coding: utf-8 -*-

from re import match
from os import makedirs, unlink
from os.path import isdir, join
from shutil import rmtree
from testcase import TestCase

class TestPuts(TestCase):

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

    def test_dirname0(self):
        self._test("""
print(dirname(\"/foo/bar\"))
""", "/foo")

    def test_dirname10(self):
        self._test("""
print(dirname(\"/foo/bar/\"))
""", "/foo")

    def test_dirname20(self):
        self._test("""
print(dirname(\"foo/bar\"))
""", "foo")

    def test_dirname30(self):
        self._test("""
print(dirname(\"foo/bar/\"))
""", "foo")

    def test_dirname40(self):
        self._test("""
print(dirname(\"foo\"))
""", ".")

    def test_dirname50(self):
        self._test("""
print(dirname(\"foo/\"))
""", ".")

    def test_dirname60(self):
        self._test("""
print(dirname(\"\"))
""", ".")

    def test_dirname70(self):
        self._test("""
print(dirname(\"/\"))
""", "/")

    def do_test_make_dirs(self, path, topdir):
        try:
            self._test("""
make_dirs(\"%(path)s\")
""" % { "path": path })
            assert isdir(path)
        finally:
            if isdir(topdir):
                rmtree(topdir)

    def test_make_dirs0(self):
        topdir = "foo"
        self.do_test_make_dirs(topdir, topdir)

    def test_make_dirs10(self):
        topdir = "foo"
        self.do_test_make_dirs(join(topdir, "bar"), topdir)

    def test_make_dirs20(self):
        topdir = "foo"
        makedirs(topdir)
        self.do_test_make_dirs(join(topdir, "bar"), topdir)

    def test_mkdir0(self):
        path = "foo"
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
