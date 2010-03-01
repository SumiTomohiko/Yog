
import os
from testcase import TestCase

class TestBuiltins(TestCase):

    disabled = os.name != "nt"

    def test_dirname0(self):
        self._test("""
print(dirname(\"c:\\\\foo\\\\bar\"))
""", "c:\\foo")

    def test_dirname10(self):
        self._test("""
print(dirname(\"c:\\\\foo\\\\bar\\\\\"))
""", "c:\\foo")

    def test_dirname20(self):
        self._test("""
print(dirname(\"foo\\\\bar\"))
""", "foo")

    def test_dirname30(self):
        self._test("""
print(dirname(\"foo\\\\bar\\\\\"))
""", "foo")

    def test_dirname40(self):
        self._test("""
print(dirname(\"foo\"))
""", ".")

    def test_dirname50(self):
        self._test("""
print(dirname(\"foo\\\\\"))
""", ".")

    def test_dirname60(self):
        self._test("""
print(dirname(\"\"))
""", ".")

    def test_dirname70(self):
        self._test("""
print(dirname(\"c:\\\\\"))
""", "c:\\")

# vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
