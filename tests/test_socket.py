# -*- coding: utf-8 -*-

from subprocess import Popen
from testcase import TestCase

class TestSocket(TestCase):

    def do_test(self, callback):
        proc = Popen(["python", "echo.py"])
        try:
            callback()
        finally:
            proc.kill()

    def test_TcpSocket0(self):
        def test():
            self._test("""
import socket
enable_gc_stress()
sock = socket.TcpSocket.new(1092)
try
  sock.send(\"foo\")
  print(sock.recv(3))
finally
  sock.close()
end
""", "\\x66\\x6f\\x6f")
        self.do_test(test)

# vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
