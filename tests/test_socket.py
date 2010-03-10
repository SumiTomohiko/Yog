# -*- coding: utf-8 -*-

from subprocess import Popen
from testcase import TestCase

class TestSocket(TestCase):

    port = 1092

    def do_test(self, callback):
        port = self.port
        self.port += 1

        proc = Popen(["python", "echo.py", str(port)])
        try:
            callback(port)
        finally:
            proc.kill()

    def test_TcpSocket0(self):
        def test(port):
            self._test("""
import socket
enable_gc_stress()
sock = socket.TcpSocket.new(\"127.0.0.1\", %(port)u)
try
  sock.send(\"foo\")
  print(sock.recv(3))
finally
  sock.close()
end
""" % locals(), "\\x66\\x6f\\x6f")
        self.do_test(test)

    def test_TcpSocket10(self):
        def test(port):
            self._test("""
import socket
enable_gc_stress()
sock = socket.TcpSocket.new(\"localhost\", %(port)u)
try
  sock.send(\"foo\")
  print(sock.recv(3))
finally
  sock.close()
end
""" % locals(), "\\x66\\x6f\\x6f")
        self.do_test(test)

# vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
