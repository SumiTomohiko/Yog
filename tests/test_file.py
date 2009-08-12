# -*- coding: utf-8 -*-

from testcase import TestCase

class TestFile(TestCase):

    def read_file(self, filename):
        fp = open(filename)
        try:
            return fp.read()
        finally:
            fp.close()

    def test_open0(self):
        filename = "foo.txt"
        foo = self.read_file(filename)

        self._test("""
file = File.open("%(filename)s", "r") do [f]
  print(f.read())
end
""" % { "filename": filename }, stdout=foo)

    def test_read0(self):
        filename = "foo.txt"
        foo = self.read_file(filename)

        self._test("""
file = File.open("%(filename)s", "r")
print(file.read())
file.close()
""" % { "filename": filename }, stdout=foo)

# vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
