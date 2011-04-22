# -*- coding: utf-8 -*-

from platform import system
from pty import CHILD, openpty
from select import select
from termios import ECHO, TCSAFLUSH, tcgetattr, tcsetattr
from tty import LFLAG
import os

from testcase import TestCase, get_command

def write(fd, data):
    while len(data) != 0:
        n = os.write(fd, data)
        data = data[n:]

def disable_echo(fd):
    when = TCSAFLUSH
    mode = tcgetattr(fd)
    mode[LFLAG] &= ~ECHO
    tcsetattr(fd, when, mode)

def read(fd):
    data = ""
    while True:
        rfds, _, _ = select([fd], [], [])
        if fd not in rfds:
            continue
        try:
            data += os.read(fd, 1024)
        except OSError:
            return data

def fork():
    master_fds = 3 * [None]
    slave_fds = 3 * [None]
    for i in range(len(master_fds)):
        master_fds[i], slave_fds[i] = openpty()
    pid = os.fork()
    if pid == CHILD:
        os.setsid()
        for fd in master_fds:
            os.close(fd)

        for i, fd in enumerate(slave_fds):
            if fd == i:
                continue
            os.dup2(fd, i)
            os.close(fd)

        tmp_fd = os.open(os.ttyname(1), os.O_RDWR)
        os.close(tmp_fd)
    else:
        for fd in slave_fds:
            os.close(fd)

    return pid, master_fds

def spawn(argv, stdin):
    pid, fds = fork()
    if pid == CHILD:
        os.execlp(argv[0], *argv)
        raise OSError() # NOTREACHED
    disable_echo(fds[0])
    write(fds[0], stdin)
    stdout = read(fds[1])
    stderr = read(fds[2])
    for fd in fds:
        os.close(fd)
    return stdout, stderr

class TestInteractive(TestCase):

    disabled = system() != "Linux"

    def replace_newlines(self, s):
        return s.replace("\r\n", "\n")

    def do_test(self, expected, actual):
        s = self.replace_newlines(actual)
        if callable(expected):
            return expected(s)
        assert expected == s

    def run_test(self, stdin, stdout="", stderr=""):
        cmd = [get_command(), "--gc-stress"]
        actual_stdout, actual_stderr = spawn(cmd, stdin + "\x04")
        self.do_test(stdout, actual_stdout)
        self.do_test(stderr, actual_stderr)

    def test_interactive0(self):
        self.run_test(stdout=""">>> 42
=> nil
>>> """, stdin="""puts(42)
""")

    def test_interactive10(self):
        self.run_test(stdout=""">>> 42
=> nil
>>> 26
=> nil
>>> """, stdin="""puts(42)
puts(26)
""")

    def test_interactive20(self):
        self.run_test(stdout=""">>> foobar
=> nil
>>> >>> """, stderr="""Traceback (most recent call last):
  File \"__main__\", line 1, in <package>
TypeError: unsupported operand type(s) for +: String and Fixnum
""", stdin="""puts(\"foo\" + \"bar\")
puts(\"foo\" + 42)
""")

    def test_interactive30(self):
        self.run_test(stdout=">>> >>> >>> ", stderr="""Traceback (most recent call last):
  File \"__main__\", line 1, in <package>
TypeError: unsupported operand type(s) for +: String and Fixnum
Traceback (most recent call last):
  File \"__main__\", line 1, in <package>
TypeError: unsupported operand type(s) for +: String and Fixnum
""", stdin="""puts(\"foo\" + 42)
puts(\"foo\" + 42)
""")

    def test_interactive40(self):
        self.run_test(stdout=""">>> => 42
>>> """, stdin="""42
""")

    def test_interactive50(self):
        self.run_test(stdout=""">>> => 68
>>> """, stdin="""42 + 26
""")

    def test_interactive60(self):
        def test_stdout(stdout):
            self._test_regexp(r""">>> => <Object [0-9a-fA-F]+>
>>> => 42
>>> => 42
""", stdout)

        self.run_test(stdout=test_stdout, stdin="""o = Object.new()
o.foo = 42
o.foo
""")

    def test_SyntaxError0(self):
        self.run_test(stdout=">>> >>> ", stderr="""SyntaxError: File "<stdin>", line 1: Invalid syntax
""", stdin="""def def
""")

    def test_array_indexing0(self):
        self.run_test(stdout=""">>> => [42]
>>> => 42
>>> """, stdin="""a = [42]
a[0]
""")

# vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
