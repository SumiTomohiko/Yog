# -*- coding: utf-8 -*-

from testcase import TestCase

class TestBinary(TestCase):

    def test_inspect0(self):
        src = "print(\"foo\".to_bin(ENCODINGS[\"utf-8\"]).inspect())"
        self._test(src, "b\"\\x66\\x6f\\x6f\\x00\"")

    def test_to_s0(self):
        src = "print(\"foo\".to_bin(ENCODINGS[\"utf-8\"]).inspect())"
        self._test(src, "b\"\\x66\\x6f\\x6f\\x00\"")
        self._test("""\
enc = ENCODINGS[\"utf-8\"]
print(\"foo\".to_bin(enc).to_s(enc))""", "foo")

# vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4 filetype=python
