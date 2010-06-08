
from unix import TestUnix

class TestBuiltins(TestUnix):

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

# vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
