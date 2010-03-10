# -*- coding: utf-8 -*-

from filecmp import cmp
from os import unlink
from os.path import join
from shutil import rmtree
from tempfile import mkdtemp
from zipfile import ZIP_DEFLATED, ZipFile
from testcase import TestCase

class TestZip(TestCase):

    options = []

    def test_gc0(self):
        self._test("""
import zip
""", options=["--gc-stress"])

    def do_compress_test(self, *args):
        zipfile = "actual.zip"
        try:
            unlink(zipfile)
        except:
            pass
        targets = ", ".join(["\"%s\"" % (file, ) for file in args])
        self._test("""
import zip
enable_gc_stress()
zip.compress(\"%(zipfile)s\", %(targets)s)
""" % locals())
        dirpath = mkdtemp("TestZip.do_compress_test")
        try:
            zf = ZipFile(zipfile)
            try:
                zf.extractall(dirpath)
            finally:
                zf.close()
            for file in args:
                assert cmp(file, join(dirpath, file))
        finally:
            rmtree(dirpath)

    def test_compress0(self):
        self.do_compress_test("gods.txt")

    def test_compress10(self):
        self.do_compress_test(join("samples", "favorites"))

    def do_decompress_test(self, *args):
        zip = "test.zip"
        zf = ZipFile(zip, "w", ZIP_DEFLATED)
        try:
            for file in args:
                zf.write(file)
        finally:
            zf.close()
        tempdir = mkdtemp("TestZip.do_decompress_test")
        try:
            self._test("""
import zip
enable_gc_stress()
zip.decompress(\"%(zip)s\", \"%(tempdir)s\")
""" % locals())
            for file in args:
                assert cmp(file, join(tempdir, file))
        finally:
            rmtree(tempdir)

    def test_decompress0(self):
        self.do_decompress_test("gods.txt")

    def test_decompress10(self):
        self.do_decompress_test(join("samples", "favorites"))

# vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
