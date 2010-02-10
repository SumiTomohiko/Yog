# -*- coding: utf-8 -*-

from os import makedirs
from os.path import join
from shutil import rmtree
from tempfile import mkdtemp
from testcase import TestCase

class TestYdoc(TestCase):

    options = []

    def read(self, path):
        fp = open(path)
        try:
            return fp.read()
        finally:
            fp.close()

    def write(self, path, s):
        fp = open(path, "w")
        try:
            fp.write(s)
        finally:
            fp.close()

    def do_test(self, src, expected):
        tmpdir = mkdtemp()
        try:
            index = join(tmpdir, "index.ydoc")
            self.write(index, src)

            destdir = join(tmpdir, "out")
            makedirs(destdir)

            self._test("""
from test_ydoc import Generator
import ydoc
enable_gc_stress()

ydoc.run(\"test\", \"%(destdir)s\", \"%(index)s\", generator: Generator)
""" % { "destdir": destdir, "index": index })

            actual = self.read(join(destdir, "index.html"))
        finally:
            #rmtree(tmpdir)
            pass

        assert expected == actual

    def test_title0(self):
        self.do_test("""
= title
""", """= title
""")

# vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
