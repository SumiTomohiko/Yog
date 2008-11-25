# -*- coding: utf-8 -*-

from os import environ, unlink
from subprocess import PIPE, Popen
from tempfile import mkstemp

class TestCase(object):

    def _test(self, src, stdout="", stderr="", status=None, options=[]):
        env_gc = environ["GC"]
        if env_gc == "COPYING":
            gc = "copying"
        elif env_gc == "MARK-SWEEP":
            gc = "mark-sweep"

        options = options or ["--always-gc", "--gc=%(gc)s" % { "gc": gc }]

        file = mkstemp(prefix="yog")[1]
        try:
            f = open(file, "w")
            try:
                f.write(src)
            finally:
                f.close()

            cmd = ["./yog"]
            cmd.extend(options)
            cmd.append(file)

            proc = Popen(cmd, stdout=PIPE, stderr=PIPE)
            proc.wait()
            if stdout is not None:
                out = proc.stdout.read()
                if callable(stdout):
                    stdout(out)
                else:
                    assert stdout == out
            if stderr is not None:
                err = proc.stderr.read()
                if callable(stderr):
                    stderr(err)
                else:
                    assert stderr == err
            if status is not None:
                returncode = proc.returncode
                if callable(status):
                    status(returncode)
                else:
                    assert status == returncode
        finally:
            unlink(file)

# vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
