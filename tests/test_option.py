# -*- coding: utf-8 -*-

from testcase import TestCase

class TestOption(TestCase):

    def _test_option(self, options):
        self._test("", options=options)

    def _test_threshold(self, threshold):
        self._test_option(["--threshold=%(threshold)s" % { "threshold": threshold }])

    def test_threshold1(self):
        self._test_threshold("42")

    def test_threshold2(self):
        self._test_threshold("42k")

    def test_threshold3(self):
        self._test_threshold("42K")

    def test_threshold4(self):
        self._test_threshold("42m")

    def test_threshold5(self):
        self._test_threshold("42M")

    def test_threshold6(self):
        self._test_threshold("42M43k44")

# vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
