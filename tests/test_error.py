# -*- coding: utf-8 -*-

from os import environ
from re import match, search
from testcase import TestCase

class TestError(TestCase):

    disabled = environ.get("GC", "copying") != "copying"

    def test_out_of_memory0(self):
        def test_stderr(stderr):
            m = search(r"out of memory", stderr)
            assert m is not None

        self._test("""
puts("xx" * 536870912)
""", stderr=test_stderr, status=None)

    def test_open0(self):
        proc = self.run_command(["foo"])
        stderr = proc.stderr.read()
        m = match("can't open file \"foo\"", stderr)
        assert m is not None

# vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
