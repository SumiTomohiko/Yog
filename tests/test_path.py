# -*- coding: utf-8 -*-

from os import getpid, lstat, makedirs, readlink, symlink, walk
from os.path import abspath, join
from shutil import rmtree
from tempfile import gettempdir, mkdtemp
from testcase import TestCase, enumerate_tuples

class TestPath(TestCase):

    def do_property_test(self, path, name, expected):
        src = "print(\"%(path)s\".to_path().%(name)s)" % locals()
        self._test(src, expected)

    def do_basename_test(self, path, expected):
        self.do_property_test(path, "basename", expected)

    def do_dirname_test(self, path, expected):
        self.do_property_test(path, "dirname", expected)

    def test_dirname0(self):
        self.do_dirname_test("foo", ".")

    def test_dirname10(self):
        self.do_dirname_test(join("foo", "bar"), "foo")

    def test_dirname20(self):
        self.do_dirname_test(join("foo", ""), ".")

    def test_dirname30(self):
        self.do_dirname_test(".", ".")

    def test_dirname40(self):
        self.do_dirname_test("..", ".")

    def test_dirname50(self):
        self.do_dirname_test("/usr", "/")

    def test_basename0(self):
        self.do_basename_test("foo", "foo")

    def test_basename10(self):
        self.do_basename_test(join("foo", "bar"), "bar")

    def test_basename20(self):
        self.do_basename_test(join("foo", ""), "foo")

    def test_basename30(self):
        self.do_basename_test(".", ".")

    def test_basename40(self):
        self.do_basename_test("..", ".")

    for i, testee in enumerate([".", "..", "foo", "/", "/foo"]):
        exec("""def test_abs{index}(self):
    self._test(\"print(\\\"{testee}\\\".to_path().abs())\", abspath(\"{testee}\"))""".format(index=10 * i, testee=testee))

    for i, testee, expected in enumerate_tuples((
        ("foo", "foo"),
        ("foo/", "foo"),
        ("foo/bar", "foo/bar"),
        ("foo//bar", "foo/bar"),
        ("/", "/"),
        ("//", "/"),
        ("/foo", "/foo"),
        ("//foo", "/foo"))):
        exec("""def test_normalize{index}(self):
    self._test(\"print(\\\"{testee}\\\".to_path().normalize())\", \"{expected}\")""".format(index=10 * i, testee=testee, expected=expected))

    def run_walk_test(self, dirs):
        def test_stdout(stdout):
            actual = sorted(stdout.rstrip().split("\n"))
            expected = sorted([name for name, _, __ in walk(tmp_dir)])
            assert len(expected) == len(actual)
            for i in range(len(expected)):
                assert expected[i] == actual[i]

        tmp_dir = mkdtemp()
        try:
            for name in dirs:
                makedirs(join(tmp_dir, name))
            src = "\"{tmp_dir}\".to_path().walk(&puts)".format(**locals())
            self._test(src, stdout=test_stdout)
        finally:
            rmtree(tmp_dir)

    for i, testee in enumerate([
        [],
        ["foo"],
        ["foo", "bar"],
        [join("foo", "bar")],
        ["foo", join("bar", "baz")]]):
        exec("""def test_walk{index}(self):
    self.run_walk_test({testee})""".format(index=10 * i, testee=testee))

    def run_dir_test(self, f, expected):
        tmp_dir = mkdtemp()
        try:
            path = join(tmp_dir, "foo")
            f(path)
            src = "print(\"{path}\".to_path().dir?)".format(**locals())
            self._test(src, expected)
        finally:
            rmtree(tmp_dir)

    def touch(self, path):
        with open(path, "w") as fp:
            pass

    def test_dir0(self):
        self.run_dir_test(makedirs, "true")

    def test_dir10(self):
        self.run_dir_test(self.touch, "false")

    def test_dir20(self):
        def nop(_):
            pass
        self.run_dir_test(nop, "false")

    def run_link_test(self, name, f):
        dir_ = gettempdir()
        head = str(getpid())
        src = join(dir_, "{head}.src".format(**locals()))
        self.touch(src)
        try:
            dest = join(dir_, "{head}.dest".format(**locals()))
            fmt = "\"{dest}\".to_path().{name}(\"{src}\")"
            self._test(fmt.format(**locals()))
            try:
                f(src, dest)
            finally:
                self.unlink(dest)
        finally:
            self.unlink(src)

    def test_link_to0(self):
        def test(src, dest):
            assert lstat(src).st_ino == lstat(dest).st_ino
        self.run_link_test("link_to", test)

    def test_symlink_to0(self):
        def test(src, dest):
            assert src == readlink(dest)
        self.run_link_test("symlink_to", test)

    def test_readlink0(self):
        src = "/foo/bar/baz/quux"
        dest = join(gettempdir(), str(getpid()))
        symlink(src, dest)
        try:
            fmt = "print(\"{dest}\".to_path().readlink())"
            self._test(fmt.format(**locals()), src)
        finally:
            self.unlink(dest)

    def run_symlink_test(self, f, expected):
        path = join(gettempdir(), str(getpid()))
        f(path)
        try:
            src = "print(\"{path}\".to_path().symlink?)".format(**locals())
            self._test(src, expected)
        finally:
            self.unlink(path)

    def test_symlink0(self):
        self.run_symlink_test(self.touch, "false")

    def test_symlink10(self):
        def f(path):
            symlink("/foo/bar/baz/quux", path)
        self.run_symlink_test(f, "true")

    def test_plus0(self):
        src = "print((\"foo\".to_path() + \"bar\".to_path()).class == Path)"
        self._test(src, "true")

    def test_divide0(self):
        self._test("print(\"/\".to_path() / \"foo\")", "/foo")

    def test_bug0(self):
        src = "print(\"/home/tom/projects/Yog/src/../lib/lex.yog\".to_path().abs())"
        self._test(src, "/home/tom/projects/Yog/lib/lex.yog")

# vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
