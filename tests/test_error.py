# -*- coding: utf-8 -*-

from os import environ, unlink
from re import match, search
from testcase import TestCase

class TestError(TestCase):

    disabled = environ.get("GC", "copying") != "copying"

    def test_out_of_memory0(self):
        def test_stderr(stderr):
            m = search(r"out of memory", stderr)
            assert m is not None

        self._test("""
puts("xx" * 134217728)
""", stderr=test_stderr, status=None)

    def test_open0(self):
        stdout_path = stderr_path = None
        try:
            stdout_path = self.make_temp_file("stdout", ".log")
            stderr_path = self.make_temp_file("stderr", ".log")
            proc = self.run_command(["Not exists"], stdout_path, stderr_path)
            self.wait_proc(proc)
            stderr = self.read(stderr_path)
            m = match("Can't open file \"foo\"", stderr)
            assert m is not None
        finally:
            for path in [stdout_path, stderr_path]:
                try:
                    unlink(path)
                except:
                    pass

    def test_NameError0(self):
        def test_stderr(stderr):
            self._test_regexp(r"""Traceback \(most recent call last\):
  File "[^"]+", line 2, in <package>
NameError: Name "foo" is not defined
""", stderr)

        self._test("""
foo
""", stderr=test_stderr)

    def test_NameError10(self):
        def test_stderr(stderr):
            self._test_regexp(r"""Traceback \(most recent call last\):
  File "[^"]+", line 6, in <package>
  File "[^"]+", line 3, in foo
NameError: Name "bar" is not defined
""", stderr)

        self._test("""
def foo()
  bar
end

foo()
""", stderr=test_stderr)

# vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
