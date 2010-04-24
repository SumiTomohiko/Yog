# -*- coding: utf-8 -*-

from zlib import compress
from testcase import TestLib

class TestZlib(TestLib):

    options = []

    def test_gc0(self):
        self._test("""
import zlib
""", options=["--gc-stress"])

    def test_compress_file0(self):
        data = []
        for b in compress("foo"):
            data.append("\\x%02x" % (ord(b), ))
        self._test("""
import zlib
enable_gc_stress()
print(zlib.compress(\"foo\"))
""", "".join(data))

    def test_decompress_file0(self):
        data = []
        for c in "foo":
            data.append("\\x%02x" % (ord(c), ))
        self._test("""
import zlib
enable_gc_stress()
print(zlib.decompress(zlib.compress(\"foo\")))
""", "".join(data))

# vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
