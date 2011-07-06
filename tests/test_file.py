# -*- coding: utf-8 -*-

from os import unlink
from testcase import TestCase

class TestFile(TestCase):

    def read_file(self, filename, size=-1):
        fp = open(filename)
        try:
            return fp.read(size)
        finally:
            fp.close()

    def test_open0(self):
        filename = "gods.txt"
        foo = self.read_file(filename)

        self._test("""
file = File.open("%(filename)s", "r") do [f]
  print(f.read())
end
""" % { "filename": filename }, stdout=foo)

    def test_open10(self):
        def test_stderr(stderr):
            self._test_regexp(r"""Traceback \(most recent call last\):
  File "[^"]+", line 2, in <package>
  File builtin, in File#open
SystemError:.*
""", stderr)

        self._test("""
File.open(\"foo\", \"r\")
""", stderr=test_stderr)

    def test_open20(self):
        filename = "gods.txt"
        foo = self.read_file(filename)

        self._test("""
File.open(\"%(filename)s\") do [f]
  print(f.read())
end
""" % { "filename": filename }, stdout=foo)

    def run_read_all_test(self, size):
        filename = "gods.txt"
        expected = self.read_file(filename)

        self._test("""file = File.open(\"{filename}\", \"r\")
print(file.read({size}))
file.close()
""".format(**locals()), stdout=expected)

    def test_read0(self):
        self.run_read_all_test("")

    def test_read5(self):
        self.run_read_all_test("nil")

    def run_read_test_of_size(self, size, filename="gods.txt"):
        expected = self.read_file(filename, size)
        self._test("""file = File.open(\"{filename}\", \"r\")
print(file.read({size}))
file.close()
""".format(**locals()), stdout=expected)

    def test_read10(self):
        self.run_read_test_of_size(1)

    def test_read20(self):
        self.run_read_test_of_size(2)

    def test_read30(self):
        self.run_read_test_of_size(0)

    def test_read33(self):
        self.run_read_test_of_size(4097)

    def test_read36(self):
        self.run_read_test_of_size(8193)

    def test_read40(self):
        filename = "gods.txt"
        expected = self.read_file(filename)
        size = len(expected) + 1

        self._test("""file = File.open(\"{filename}\", \"r\")
print(file.read({size}))
file.close()
""".format(**locals()), stdout=expected)

    def test_readline0(self):
        filename = "gods.txt"

        fp = open(filename)
        try:
            line = fp.readline()
        finally:
            fp.close()

        self._test("""
File.open("%(filename)s", "r") do [f]
  print(f.readline())
end
""" % { "filename": filename }, stdout=line)

    def test_readline10(self):
        filename = "gods.txt"
        self._test("""
File.open("%(filename)s", "r") do [f]
  f.readline()
  f.readline()
  f.readline()
  f.readline()
  f.readline()
  f.readline()
  print(f.readline())
end
""" % { "filename": filename }, "nil")

    def test_write0(self):
        filename = "test_write0.tmp"

        def test_stdout(ignored):
            fp = open(filename)
            try:
                assert fp.read() == "foobarbazquux"
            finally:
                fp.close()

        try:
            unlink(filename)
        except OSError:
            pass
        self._test("""
File.open("%(filename)s", "w") do [f]
  f.write("foobarbazquux")
end
""" % { "filename": filename }, stdout=test_stdout)

# vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
