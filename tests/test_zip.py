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

    def test_compress0(self):
        zipfile = "actual.zip"
        try:
            unlink(zipfile)
        except:
            pass
        target = "gods.txt"
        self._test("""
import zip
enable_gc_stress()
zip.compress(\"%(zipfile)s\", \"%(target)s\")
""" % locals())
        dirpath = mkdtemp("TestZip.test_compress0")
        try:
            zf = ZipFile(zipfile)
            try:
                zf.extractall(dirpath)
            finally:
                zf.close()
            assert cmp(target, join(dirpath, target))
        finally:
            rmtree(dirpath)

    def test_decompress0(self):
        sample = "gods.zip"
        zf = ZipFile(sample, "w", ZIP_DEFLATED)
        try:
            target = "gods.txt"
            zf.write(target)
        finally:
            zf.close()
        tempdir = mkdtemp("TestZip.test_decompress0")
        try:
            self._test("""
import zip
enable_gc_stress()
zip.decompress(\"%(sample)s\", \"%(tempdir)s\")
""" % locals())
            assert cmp(target, join(tempdir, target))
        finally:
            rmtree(tempdir)

# vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
