# -*- coding: utf-8 -*-

from re import match
from testcase import TestCase

class TestImport(TestCase):

    def test_import1(self):
        self._test("import test_package1", "42\n")

    def test_import3(self):
        self._test("""
import test_package3

test_package3.foo()""", """42
""")

    def test_import10(self):
        self._test("""
import test_package3 as bar

bar.foo()
""", """42
""")

    def test_from0(self):
        self._test("""
from test_package3 import foo
foo()
""", """42
""")

    def test_from10(self):
        self._test("""
from test_package3 import foo as bar
bar()
""", """42
""")

    def test_from20(self):
        self._test("""
from test_package7 import foo, bar
print(foo)
""", "42")

    def test_from30(self):
        self._test("""
from test_package7 import foo, bar
print(bar)
""", "26")

    def test_from40(self):
        self._test("""
from test_package7 import foo as baz, bar
print(baz)
""", "42")

    def test_dl1(self):
        self._test("import test_package4", "42\n")

    def test_dl2(self):
        self._test("""
import test_package5

test_package5.foo()""", """42
""")

    def test_ImportError0(self):
        def test_stderr(stderr):
            self._test_regexp(r"""Traceback \(most recent call last\):
  File "[^"]+", line 2, in <package>
  File builtin, in import_package
ImportError: no package named "foo"
""", stderr)

        self._test("""
import foo
""", stderr=test_stderr)

    def test_ImportError10(self):
        def test_stderr(stderr):
            self._test_regexp(r"""Traceback \(most recent call last\):
  File "[^"]+", line 2, in <package>
  File builtin, in import_package
ImportError: dynamic package does not define init function \(YogInit_test_package6\)
""", stderr)

        self._test("""
import test_package6
""", stderr=test_stderr)

    def test_directory0(self):
        self._test("""
import test_package8
print(test_package8.foo)
""", "42")

    def test_directory10(self):
        self._test("""
import test_package8.foo
print(test_package8.foo.bar)
""", "42")

# vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
