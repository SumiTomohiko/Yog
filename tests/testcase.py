# -*- coding: utf-8 -*-

from signal import SIGKILL
from os import environ, kill, unlink
from os.path import join
from subprocess import PIPE, Popen
from tempfile import mkstemp
from time import time

class TestCase(object):

    def remove_gc_warings(self, out):
        s = out.split("\n")
        t = []
        for line in out.split("\n"):
            if line.startswith("GC Warning:"):
                continue
            t.append(line)
        return "\n".join(t)

    def get_command(self):
        try:
            return environ["YOG"]
        except KeyError:
            try:
                env_gc = environ["GC"]
            except KeyError:
                env_gc = "copying"
            env2cmd = {
                    "copying": "yog-copying",
                    "mark-sweep": "yog-mark-sweep",
                    "mark-sweep-compact": "yog-mark-sweep-compact",
                    "bdw": "yog-bdw",
                    "generational": "yog-generational" }
            return join("..", "src", env2cmd[env_gc])

    def run_command(self, args):
        cmd = [self.get_command()] + args
        return Popen(cmd, stdin=PIPE, stdout=PIPE, stderr=PIPE)

    def do(self, stdout, stderr, stdin, status, args, timeout):
        proc = self.run_command(args)
        if stdin is not None:
            proc.stdin.write(stdin)
        proc.stdin.close()

        time_begin = time()
        while True:
            if proc.poll() is not None:
                break
            if timeout < time() - time_begin:
                kill(proc.pid, SIGKILL)
                assert False, "time is out."

        if stderr is not None:
            err = self.remove_gc_warings(proc.stderr.read())
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

    def write_source(self, path, src):
        f = open(path, "w")
        try:
            f.write(src)
        finally:
            f.close()

    def _test_source(self, src, stdout, stderr, stdin, status, options, timeout, remove_tmpfile=True, tmpfile=None, yog_option=[]):
        if tmpfile is None:
            file = mkstemp(prefix="yog", suffix=".yg")[1]
        else:
            file = tmpfile
        try:
            self.write_source(file, src)
            args = options + [file] + yog_option
            self.do(stdout, stderr, stdin, status, args, timeout)
        finally:
            if remove_tmpfile:
                unlink(file)

    def _test_interactive(self, stdout, stderr, stdin, status, options, timeout):
        self.do(stdout, stderr, stdin, status, options, timeout)

    def _test(self, src=None, stdout="", stderr="", stdin=None, status=0, options=[], timeout=120, remove_tmpfile=True, tmpfile=None, yog_option=[]):
        options = options or ["--gc-stress"]

        if src is not None:
            self._test_source(src, stdout, stderr, stdin, status, options, timeout, remove_tmpfile, tmpfile, yog_option)
        else:
            self._test_interactive(stdout, stderr, stdin, status, options, timeout)

# vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
