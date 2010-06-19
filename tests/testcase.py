# -*- coding: utf-8 -*-

from re import search
from os import close, environ, unlink
from os.path import basename, exists, join, splitext
from subprocess import PIPE, Popen
from tempfile import mkstemp
from time import localtime, strftime, time
import os
import sys

def find_so(name):
    so = "lib%s.so" % (name, )
    try:
        path = environ["LD_LIBRARY_PATH"]
    except KeyError:
        path = ""
    dirs = [d for d in path.split(":") if 0 < len(d)] + ["/usr/local/lib", "/usr/lib", "/lib"]
    for d in dirs:
        if exists(join(d, so)):
            return True
    return False

def get_lib_path():
    return join(".", "test_lib" + (".so" if os.name == "posix" else ".dll")).replace("\\", "\\\\")

def get_command():
    try:
        return environ["YOG"]
    except KeyError:
        return join("..", "src", "yog-generational")

class TestCase(object):

    def kill_proc(self, proc):
        try:
            proc.kill()
        except AttributeError:
            pass
        else:
            return
        from os import kill
        from signal import SIGKILL
        kill(proc.pid, SIGKILL)

    def remove_gc_warings(self, out):
        s = out.split("\n")
        t = []
        for line in out.split("\n"):
            if line.startswith("GC Warning:"):
                continue
            t.append(line)
        return "\n".join(t)

    def run_command(self, args, stdout_path, stderr_path):
        stdout = open(stdout_path, "w")
        try:
            stderr = open(stderr_path, "w")
            try:
                cmd = [get_command()] + args
                return Popen(cmd, stdin=PIPE, stdout=stdout, stderr=stderr)
            finally:
                stderr.close()
        finally:
            stdout.close()

    def format_time(self, sec):
        return strftime("%x %X", localtime(sec))

    def _test_regexp(self, regexp, s):
        m = search(regexp, s)
        assert m is not None
        return m

    def read(self, path):
        fp = open(path)
        try:
            return fp.read()
        finally:
            fp.close()

    def wait_proc(self, proc, timeout=5 * 60):
        time_begin = time()
        while True:
            if proc.poll() is not None:
                break
            now = time()
            if timeout < now - time_begin:
                self.kill_proc(proc)
                assert False, "time is out (starting at %s, now is %s)" % (self.format_time(time_begin), self.format_time(now))

    def do(self, stdout, stderr, stdin, status, args, timeout, encoding=None):
        stdout_path = stderr_path = None
        ext = ".log"
        try:
            stdout_path = self.make_temp_file(prefix="stdout", suffix=ext)
            stderr_path = self.make_temp_file(prefix="stderr", suffix=ext)
            proc = self.run_command(args, stdout_path, stderr_path)
            if stdin is not None:
                proc.stdin.write(stdin)
            proc.stdin.close()
            self.wait_proc(proc, timeout)

            err = self.remove_gc_warings(self.read(stderr_path))
            print >> sys.stderr, err
            out = self.read(stdout_path)
            print out
            if stderr is not None:
                if encoding is not None:
                    err = err.decode(encoding)
                if callable(stderr):
                    stderr(err)
                else:
                    assert stderr == err, "stderr must be %r, but actual is %r" % (stderr, err)

            if stdout is not None:
                if encoding is not None:
                    out = out.decode(encoding)
                if callable(stdout):
                    stdout(out)
                else:
                    assert stdout == out, "stdout must be %r, but actual is %r" % (stdout, out)

            if status is not None:
                returncode = proc.returncode
                if callable(status):
                    status(returncode)
                else:
                    assert status == returncode
        finally:
            for path in [stdout_path, stderr_path]:
                if path is not None:
                    unlink(path)

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

    def make_temp_file(self, prefix="yog", suffix=".yg"):
        fd, path = mkstemp(prefix=prefix, suffix=suffix)
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

    def _test(self, src=None, stdout="", stderr="", stdin=None, status=0, options=None, timeout=5 * 60, remove_tmpfile=True, tmpfile=None, yog_option=[], encoding="UTF-8"):
        if options is None:
            try:
                options = self.options
            except AttributeError:
                options = ["--gc-stress"]

        if src is not None:
            self._test_source(src, stdout, stderr, stdin, status, options, timeout, remove_tmpfile, tmpfile, yog_option, encoding)
        else:
            self._test_interactive(stdout, stderr, stdin, status, options, timeout)

class TestLib(TestCase):

    disabled = splitext(basename(get_command()))[0] != "yog-generational"

# vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
