# -*- coding: utf-8 -*-

from os import unlink
from testcase import TestCase

class Base(TestCase):

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
        proc = self.run_command(["--young-heap-size=48M", path])
        self.wait_proc(proc)

    def do_test(self, headers, so, src, expected):
        self.unlink("test_h2yog.yg")
        self.run_h2yog(headers, so)
        self._test(src, expected)

# vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
