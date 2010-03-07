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

    def test_load_map0(self):
        self._test("""
import yaml
enable_gc_stress()
d = yaml.load_string(<<EOF)
42: 26
foo: bar
EOF
print(d[42])
""", "26")

    def test_load_map10(self):
        self._test("""
import yaml
enable_gc_stress()
d = yaml.load_string(<<EOF)
42: 26
foo: bar
EOF
print(d[\"foo\"])
""", "bar")

# vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
