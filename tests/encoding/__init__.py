# -*- coding: utf-8 -*-

from testcase import TestCase

class TestEncoding(TestCase):

    encoding = None

    def _test_encoding(self, src, stdout):
        self._test(src, stdout, encoding=self.encoding)

# vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
