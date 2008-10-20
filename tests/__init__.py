# -*- coding: utf-8 -*-

from os import unlink
from subprocess import PIPE, Popen
from tempfile import mkstemp

class TestCase(object):

    def _test(self, src, stdout=None, stderr=None, status=None):
        file = mkstemp(prefix="yog")[1]
        try:
            f = open(file, "w")
            try:
                f.write(src)
            finally:
                f.close()

            proc = Popen(["./yog", file], stdout=PIPE, stderr=PIPE)
            proc.wait()
            if stdout is not None:
                out = proc.stdout.read()
                assert stdout == out
            if stderr is not None:
                err = proc.stderr.read()
                assert stderr == err
            if status is not None:
                assert status == proc.returncode
        finally:
            unlink(file)

# vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
