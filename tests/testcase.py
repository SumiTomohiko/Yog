# -*- coding: utf-8 -*-

from re import match
from os import close, environ, unlink
from os.path import join
from subprocess import PIPE, Popen
from tempfile import mkstemp
from time import localtime, strftime, time
import os

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
        if os.name == "nt":
            return join("..", "vs", "2003", "Yog", "yog.exe")
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

    def terminate_process(self, pid):
        from signal import SIGKILL
        from os import kill
        kill(pid, SIGKILL)

    def conv_newline(self, s):
        t = []
        for line in s.split("\n"):
            t.append(line)

        if os.name == "nt":
            newline = "\r\n"
        else:
            newline = "\n"
        return newline.join(t)

    def format_time(self, sec):
        return strftime("%x %X", sec)

    def _test_regexp(self, regexp, s):
        m = match(self.conv_newline(regexp), s)
        assert m is not None
        return m

    def do(self, stdout, stderr, stdin, status, args, timeout, encoding=None):
        proc = self.run_command(args)
        if stdin is not None:
            proc.stdin.write(stdin)
        proc.stdin.close()

        time_begin = time()
        while True:
            if proc.poll() is not None:
                break
            now = time()
            if timeout < now - time_begin:
                self.terminate_process(proc.pid)
                assert False, "time is out (starting at %s, now is %s)" % (self.format_time(time_begin), self.format_time(now))

        if stderr is not None:
            err = self.remove_gc_warings(proc.stderr.read())
            if encoding is not None:
                err = err.decode(encoding)
            if callable(stderr):
                stderr(err)
            else:
                stderr = self.conv_newline(stderr)
                assert stderr == err

        if stdout is not None:
            out = proc.stdout.read()
            if encoding is not None:
                out = out.decode(encoding)
            if callable(stdout):
                stdout(out)
            else:
                stdout = self.conv_newline(stdout)
                assert stdout == out

        if status is not None:
            returncode = proc.returncode
            if callable(status):
                status(returncode)
            else:
                assert status == returncode

    def write_source(self, path, src, encoding):
        if encoding is not None:
            import codecs
            f = codecs.open(path, "w", encoding)
        else:
            f = open(path, "w")
        try:
            f.write(src)
        finally:
            f.close()

    def make_temp_file(self):
        fd, path = mkstemp(prefix="yog", suffix=".yg")
        close(fd)
        return path

    def _test_source(self, src, stdout, stderr, stdin, status, options, timeout, remove_tmpfile=True, tmpfile=None, yog_option=[], encoding=None):
        file = tmpfile or self.make_temp_file()
        try:
            self.write_source(file, src, encoding)
            args = options + [file] + yog_option
            self.do(stdout, stderr, stdin, status, args, timeout, encoding)
        finally:
            if remove_tmpfile:
                try:
                    unlink(file)
                except:
                    pass

    def _test_interactive(self, stdout, stderr, stdin, status, options, timeout):
        self.do(stdout, stderr, stdin, status, options, timeout)

    def _test(self, src=None, stdout="", stderr="", stdin=None, status=0, options=[], timeout=5 * 60, remove_tmpfile=True, tmpfile=None, yog_option=[], encoding="UTF-8"):
        options = options or ["--gc-stress"]

        if src is not None:
            self._test_source(src, stdout, stderr, stdin, status, options, timeout, remove_tmpfile, tmpfile, yog_option, encoding)
        else:
            self._test_interactive(stdout, stderr, stdin, status, options, timeout)

# vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
