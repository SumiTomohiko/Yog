# -*- coding: utf-8 -*-

from testcase import TestCase, enumerate_tuples

class TestBase64(TestCase):

    options = []

    for i, src, expected in enumerate_tuples((
            ("AAAA", "0x00 0x00 0x00"),
            ("AAAB", "0x00 0x00 0x01"),
            ("AABA", "0x00 0x00 0x40"),
            ("ABAA", "0x00 0x10 0x00"),
            ("BAAA", "0x04 0x00 0x00"),
            ("AAAAAAAA", "0x00 0x00 0x00 0x00 0x00 0x00"),
            ("AAAA\\\\nAAAA", "0x00 0x00 0x00 0x00 0x00 0x00"),
            ("AA==", "0x00"),
            ("BA==", "0x04"),
            ("CA==", "0x08"),
            ("DA==", "0x0c"),
            ("EA==", "0x10"),
            ("FA==", "0x14"),
            ("GA==", "0x18"),
            ("HA==", "0x1c"),
            ("IA==", "0x20"),
            ("JA==", "0x24"),
            ("KA==", "0x28"),
            ("LA==", "0x2c"),
            ("MA==", "0x30"),
            ("NA==", "0x34"),
            ("OA==", "0x38"),
            ("PA==", "0x3c"),
            ("QA==", "0x40"),
            ("RA==", "0x44"),
            ("SA==", "0x48"),
            ("TA==", "0x4c"),
            ("UA==", "0x50"),
            ("VA==", "0x54"),
            ("WA==", "0x58"),
            ("XA==", "0x5c"),
            ("YA==", "0x60"),
            ("ZA==", "0x64"),
            ("aA==", "0x68"),
            ("bA==", "0x6c"),
            ("cA==", "0x70"),
            ("dA==", "0x74"),
            ("eA==", "0x78"),
            ("fA==", "0x7c"),
            ("gA==", "0x80"),
            ("hA==", "0x84"),
            ("iA==", "0x88"),
            ("jA==", "0x8c"),
            ("kA==", "0x90"),
            ("lA==", "0x94"),
            ("mA==", "0x98"),
            ("nA==", "0x9c"),
            ("oA==", "0xa0"),
            ("pA==", "0xa4"),
            ("qA==", "0xa8"),
            ("rA==", "0xac"),
            ("sA==", "0xb0"),
            ("tA==", "0xb4"),
            ("uA==", "0xb8"),
            ("vA==", "0xbc"),
            ("wA==", "0xc0"),
            ("xA==", "0xc4"),
            ("yA==", "0xc8"),
            ("zA==", "0xcc"),
            ("0A==", "0xd0"),
            ("1A==", "0xd4"),
            ("2A==", "0xd8"),
            ("3A==", "0xdc"),
            ("4A==", "0xe0"),
            ("5A==", "0xe4"),
            ("6A==", "0xe8"),
            ("7A==", "0xec"),
            ("8A==", "0xf0"),
            ("9A==", "0xf4"),
            ("+A==", "0xf8"),
            ("/A==", "0xfc"),
            ("AQ==", "0x01"),
            ("Ag==", "0x02"),
            ("Aw==", "0x03"),
            ("AAA=", "0x00 0x00"),
            ("AAE=", "0x00 0x01"),
            ("AAI=", "0x00 0x02"),
            ("AAM=", "0x00 0x03"),
            ("AAQ=", "0x00 0x04"),
            ("AAU=", "0x00 0x05"),
            ("AAY=", "0x00 0x06"),
            ("AAc=", "0x00 0x07"),
            ("AAg=", "0x00 0x08"),
            ("AAk=", "0x00 0x09"),
            ("AAo=", "0x00 0x0a"),
            ("AAs=", "0x00 0x0b"),
            ("AAw=", "0x00 0x0c"),
            ("AA0=", "0x00 0x0d"),
            ("AA4=", "0x00 0x0e"),
            ("AA8=", "0x00 0x0f"))):
        exec("""def test_to_bin{i}(self):
    self._test(\"\"\"from base64 import base64_to_bin
print(base64_to_bin(\\\"{src}\\\").to_a().map(&get_attr(\\\'to_s) + apply(16) + \\\"0x{{0:02}}\\\".format).join(\\\" \\\"))\"\"\", \"{expected}\")
""".format(i=10 * i, src=src, expected=expected))

    for i, src, expected in enumerate_tuples((("Zm9v", "foo"), )):
        exec("""def test_to_s{i}(self):
    self._test(\"\"\"from base64 import base64_to_s
print(base64_to_s(\\\"{src}\\\", ENCODINGS[\\\"utf-8\\\"]))\"\"\", \"{expected}\")
""".format(i=10 * i, src=src, expected=expected))

# vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4 filetype=python
