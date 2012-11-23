# -*- coding: utf-8 -*-

from testcase import TestCase, enumerate_tuples

class TestQuotedPrintable(TestCase):

    options = []

    for i, qp, expected in enumerate_tuples((
            ("foo", "0x66 0x6f 0x6f"),
            ("foo=3D", "0x66 0x6f 0x6f 0x3d"),
            ("=3D", "0x3d"),
            ("=3Dfoo", "0x3d 0x66 0x6f 0x6f"),
            ("=3D=3D", "0x3d 0x3d"),
            ("foo=\\\\r\\\\nbar", "0x66 0x6f 0x6f 0x62 0x61 0x72"),
            ("foo= \\\\r\\\\nbar", "0x66 0x6f 0x6f 0x62 0x61 0x72"),
            ("foo =\\\\r\\\\nbar", "0x66 0x6f 0x6f 0x20 0x62 0x61 0x72"),
            ("\\\\r\\\\n", "0x0d 0x0a"),
            ("foo\\\\r\\\\nbar", "0x66 0x6f 0x6f 0x0d 0x0a 0x62 0x61 0x72"))):
        exec("""def test_quotedprintable_to_bin{i}(self):
    self._test(\"\"\"from quotedprintable import quotedprintable_to_bin
print(quotedprintable_to_bin(\\\"{qp}\\\").to_a().map(&get_attr(\\\'to_s) + apply(16)).map(&\\\"0x{{0:02}}\\\".format).join(\\\" \\\"))\"\"\", \"{expected}\")
""".format(i=10 * i, qp=qp, expected=expected))

    for i, qp, expected in enumerate_tuples((
            ("foo", "foo"),
            ("=3D", "="))):
        exec("""def test_quotedprintable_to_s{i}(self):
    self._test(\"\"\"from quotedprintable import quotedprintable_to_s
enc = ENCODINGS[\\\"utf-8\\\"]
print(quotedprintable_to_s(\\\"{qp}\\\", enc))\"\"\", \"{expected}\")
""".format(i=10 * i, qp=qp, expected=expected))

# vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4 filetype=python
