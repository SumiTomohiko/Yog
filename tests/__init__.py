# -*- coding: utf-8 -*-

from signal import SIGKILL
from os import environ, kill, unlink
from subprocess import PIPE, Popen
from tempfile import mkstemp
from time import time

class TestCase(object):

    def _test(self, src, stdout="", stderr="", status=None, options=[], timeout=5):
        env_gc = environ["GC"]
        if env_gc == "copying":
            gc = "copying"
        elif env_gc == "mark-sweep":
            gc = "mark-sweep"
        elif env_gc == "bdw":
            gc = "bdw"

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
            time_begin = time()
            while True:
                if proc.poll() is not None:
                    break
                if timeout < time() - time_begin:
                    kill(proc.pid, SIGKILL)
                    assert False, "time is out."

            if stderr is not None:
                err = proc.stderr.read()
                if callable(stderr):
                    stderr(err)
                else:
                    assert stderr == err
            if stdout is not None:
                out = proc.stdout.read()
                if callable(stdout):
                    stdout(out)
                else:
                    assert stdout == out
            if status is not None:
                returncode = proc.returncode
                if callable(status):
                    status(returncode)
                else:
                    assert status == returncode
        finally:
            unlink(file)

# vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
