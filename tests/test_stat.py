# -*- coding: utf-8 -*-

from os import chmod, close, unlink
from os.path import basename
from tempfile import mkstemp
from testcase import TestCase

class TestStat(TestCase):

    def run_mode_test(self, mode):
        fd, path = mkstemp(prefix=basename(__file__))
        try:
            close(fd)
            chmod(path, mode)
            fmt = "print(\"{path}\".to_path().lstat().mode)".format(**locals())
            self._test(fmt, str(mode))
        finally:
            unlink(path)

    for i, mode in enumerate([0644, 0755]):
        exec """def test_mode{index}(self):
    self.run_mode_test({mode})""".format(index=10 * i, mode=mode)

# vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
