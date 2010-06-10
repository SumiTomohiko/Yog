# -*- coding: utf-8 -*-

from sys import platform
from zlib import compress
from unix import TestUnix

class TestZlib(TestUnix):

    disabled = TestUnix.disabled or (0 <= platform.find("openbsd"))
    options = []

    def test_gc0(self):
        self._test("""
import zlib
""", options=["--gc-stress"])

    def format_binary(self, s):
        return "b\"" + s + "\""

    def test_compress_file0(self):
        data = []
        for b in compress("foo"):
            data.append("\\x%02x" % (ord(b), ))
        self._test("""
import zlib
enable_gc_stress()
print(zlib.compress(\"foo\".to_bin()))
""", self.format_binary("".join(data)))

    def test_decompress_file0(self):
        data = []
        for c in "foo":
            data.append("\\x%02x" % (ord(c), ))
        self._test("""
import zlib
enable_gc_stress()
print(zlib.decompress(zlib.compress(\"foo\".to_bin())))
""", self.format_binary("".join(data)))

# vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
