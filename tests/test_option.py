# -*- coding: utf-8 -*-

from tests import TestCase

class TestOption(TestCase):

    def _test_option(self, options):
        self._test("", options=options)

    def _test_init_heap_size(self, size):
        self._test_option(["--gc=copying", "--init-heap-size=%(size)s" % { "size": size }])

    def test_init_heap_size1(self):
        self._test_init_heap_size("42")

    def test_init_heap_size2(self):
        self._test_init_heap_size("42k")

    def test_init_heap_size3(self):
        self._test_init_heap_size("42K")

    def test_init_heap_size4(self):
        self._test_init_heap_size("42m")

    def test_init_heap_size5(self):
        self._test_init_heap_size("42M")

    def test_init_heap_size6(self):
        self._test_init_heap_size("42M43k44")

    def _test_threshold(self, threshold):
        self._test_option(["--gc=mark-sweep", "--threshold=%(threshold)s" % { "threshold": threshold }])

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
