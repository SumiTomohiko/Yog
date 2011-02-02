# -*- coding: utf-8 -*-

from os import unlink
from testcase import TestCase

class TestH2Yog(TestCase):

    def run_h2yog(self, headers, so):
        src = """from h2yog import h2yog
headers = [%(headers)s].map() do [h]
  next h.to_path()
end
h2yog(\"test_h2yog.yg\", headers, \"%(so)s\") do [path, name]
  next headers.include?(path.basename)
end""" % { "headers": ", ".join([ "\"%s\"" % (header, ) for header in headers]), "so": so }
        path = "run_h2yog.yg"
        unlink(path)
        self.write_source(path, src)
        self.run_command([path])

    def do_test(self, headers, so, src, expected):
        self.run_h2yog(headers, so)
        self._test(src, expected)

    def test_constant0(self):
        headers = ["test_constant0.h"]
        so = "empty"
        src = """from test_h2yog import FOO
print(FOO)"""
        self.do_test(headers, so, src, "42")

# vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
