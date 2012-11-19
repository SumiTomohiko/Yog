# -*- coding: utf-8 -*-

from sys import platform
from zlib import compress
import pytest
from testcase import find_so
from unix import TestUnix

@pytest.mark.skipif("not find_so(\"z\")")
class TestZlib(TestUnix):

    options = []

    def test_gc0(self):
        self._test("""
import zlib
""", options=["--gc-stress"])

    def format_binary(self, s):
        return "b\"" + s + "\""

    def test_compress_file0(self):
        data = []
        for b in compress("foo\x00"):
            data.append("\\x%02x" % (ord(b), ))
        self._test("""
import zlib
enable_gc_stress()
print(zlib.compress(\"foo\".to_cstr(ENCODINGS[\"utf-8\"])))
""", self.format_binary("".join(data)))

    def test_decompress_file0(self):
        data = []
        for c in "foo\x00":
            data.append("\\x%02x" % (ord(c), ))
        self._test("""
import zlib
enable_gc_stress()
print(zlib.decompress(zlib.compress(\"foo\".to_cstr(ENCODINGS[\"utf-8\"]))))
""", self.format_binary("".join(data)))

# vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
