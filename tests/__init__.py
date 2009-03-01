# -*- coding: utf-8 -*-

from signal import SIGKILL
from os import environ, kill, unlink
from subprocess import PIPE, Popen
from tempfile import mkstemp
from time import time

class TestCase(object):

    def _test(self, src, stdout="", stderr="", status=0, options=[], timeout=30, remove_tmpfile=True):
        try:
            env_gc = environ["GC"]
        except KeyError:
            env_gc = "copying"
        if env_gc == "copying":
            gc = "copying"
        elif env_gc == "mark-sweep":
            gc = "mark-sweep"
        elif env_gc == "mark-sweep-compact":
            gc = "mark-sweep-compact"
        elif env_gc == "bdw":
            gc = "bdw"

        options = options or ["--gc-stress", "--gc=%(gc)s" % { "gc": gc }]

        file = mkstemp(prefix="yog", suffix=".yg")[1]
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

            returncode = proc.returncode
            if callable(status):
                status(returncode)
            else:
                assert status == returncode
        finally:
            if remove_tmpfile:
                unlink(file)

# vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
