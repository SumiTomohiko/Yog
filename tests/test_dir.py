# -*- coding: utf-8 -*-

from os.path import join
from shutil import rmtree
from tempfile import mkdtemp
from testcase import TestCase

class TestDir(TestCase):

    def touch(self, dir_, name):
        with open(join(dir_, name), "w") as fp:
            pass

    def run_open_test(self, expected):
        tmp_dir = mkdtemp()
        try:
            for e in expected:
                self.touch(tmp_dir, e)
            self._test("""Dir.open(\"{tmp_dir}\") do |dir|
  dir.each(&print)
end""".format(**locals()), "".join([".", ".."] + expected))
        finally:
            rmtree(tmp_dir)

    for i, testee in enumerate([[], ["foo"], ["foo", "bar"]]):
        exec("""def test_open{index}(self):
    self.run_open_test({testee})""".format(index=10 * i, testee=testee))

# vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
