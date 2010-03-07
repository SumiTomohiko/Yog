# -*- coding: utf-8 -*-

from testcase import TestCase

class TestYaml(TestCase):

    options = []

    def test_gc0(self):
        self._test("import yaml", options=["--gc-stress"])

    def test_load_string0(self):
        self._test("""
import yaml
enable_gc_stress()
print(yaml.load_string(<<EOF))
- 42
- 26
EOF
""", "[42, 26]")

# vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
