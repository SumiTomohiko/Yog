# -*- coding: utf-8 -*-

from re import match
from testcase import TestCase

class TestImport(TestCase):

    def test_import1(self):
        self._test("import test_package1", "42\n")

    def test_import2(self):
        self._test("import test_package1, test_package2", "42\n26\n")

    def test_import3(self):
        self._test("""
import test_package3

test_package3.foo()""", """42
""")

    def test_dl1(self):
        self._test("import test_package4", "42\n")

    def test_dl2(self):
        self._test("""
import test_package5

test_package5.foo()""", """42
""")

    def test_ImportError0(self):
        def test_stderr(stderr):
            m = match(r"""Traceback \(most recent call last\):
  File "[^"]+", line 2, in <module>
  File builtin, in import_package
ImportError: no package named 'foo'
""", stderr)
            assert m is not None

        self._test("""
import foo
""", stderr=test_stderr)

    def test_ImportError10(self):
        def test_stderr(stderr):
            m = match(r"""Traceback \(most recent call last\):
  File "[^"]+", line 2, in <module>
  File builtin, in import_package
ImportError: dynamic package does not define init function (YogInit_test_package6)",
""", stderr)
            assert m is not None

        self._test("""
import test_package6
""", stderr=test_stderr)

# vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
