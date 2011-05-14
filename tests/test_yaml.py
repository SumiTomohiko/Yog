# -*- coding: utf-8 -*-

from testcase import TestLib, find_so
import pytest

@pytest.mark.skipif("not find_so(\"syck\")")
class TestYaml(TestLib):

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

    def test_dump_to_string0(self):
        self._test("""
import yaml
enable_gc_stress()
print(yaml.dump_to_string([42]))
""", """---\x20
- 42
""")

# vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
