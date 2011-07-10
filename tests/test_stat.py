# -*- coding: utf-8 -*-

from os import chmod, close, stat, unlink
from os.path import basename
from tempfile import mkstemp
from testcase import TestCase

class TestStat(TestCase):

    def run_mode_test(self, mode):
        path = self.make_temp_file(prefix=basename(__file__))
        try:
            chmod(path, mode)
            src = "print(\"{path}\".to_path().lstat().mode)".format(**locals())
            self._test(src, str(mode))
        finally:
            unlink(path)

    for i, mode in enumerate([0644, 0755]):
        exec """def test_mode{index}(self):
    self.run_mode_test({mode})""".format(index=10 * i, mode=mode)

    def run_id_test(self, name):
        # TODO: Why dose not Python have a temporary file class for with
        # statement?
        path = self.make_temp_file(prefix=basename(__file__))
        try:
            attr = "st_{name}".format(**locals())
            fmt = "print(\"{path}\".to_path().lstat().{name})"
            self._test(fmt.format(**locals()), str(getattr(stat(path), attr)))
        finally:
            unlink(path)

    for name in ["uid", "gid"]:
        exec """def test_{name}0(self):
    self.run_id_test(\"{name}\")""".format(name=name)

# vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
