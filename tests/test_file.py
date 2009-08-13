# -*- coding: utf-8 -*-

from os import unlink
from re import match
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

    def test_readline0(self):
        filename = "foo.txt"

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
        def test_stderr(stderr):
            m = match(r"""Traceback \(most recent call last\):
  File "[^"]+", line 2, in <module>
  File builtin, in File#open
  File "[^"]+", line 4, in <block>
  File builtin, in File#readline
EOFError: end of file reached
""", stderr)
            assert m is not None

        filename = "foo.txt"
        self._test("""
File.open("%(filename)s", "r") do [f]
  while true
    f.readline()
  end
end
""" % { "filename": filename }, stderr=test_stderr)

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
