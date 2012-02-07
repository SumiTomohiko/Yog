# -*- coding: utf-8 -*-

from testcase import TestCase, get_lib_path

class Base(TestCase):

    def do_mode_test(self, type, mode, expected):
        fmt = "typedef {0} Foo __attribute__((__mode__({1})));"
        self.do_simple_print_test(fmt.format(type, mode), expected, "Foo")

    def do_simple_print_test(self, header, expected, name="FOO"):
        self.do_test2(header, """from test_h2yog import {0}
print({0})""".format(name), expected)

    def run_h2yog(self, headers, so):
        src = """from h2yog import h2yog
headers = [%(headers)s].map() do |h|
  next h.to_path()
end
h2yog(\"test_h2yog.yog\", headers, \"%(so)s\") do |path, name|
  next headers.include?(path.basename)
end""" % { "headers": ", ".join([ "\"%s\"" % (header, ) for header in headers]), "so": so }
        path = "run_h2yog.yog"
        self.unlink(path)
        self.write_source(path, src)
        proc = self.run_command(["--young-heap-size=48M", path])
        self.wait_proc(proc)

    def do_test(self, headers, so, src, expected):
        self.unlink("test_h2yog.yog")
        self.run_h2yog(headers, so)
        self._test(src, expected)

    def do_test2(self, header, src, expected):
        path = "h2yog.h"
        self.write_source(path, header)
        self.do_test([path], get_lib_path("empty"), src, expected)

# vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
