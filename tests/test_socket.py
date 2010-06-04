# -*- coding: utf-8 -*-

from subprocess import Popen
from testcase import TestLib

class TestSocket(TestLib):

    port = 1092

    def do_test(self, callback):
        port = self.__class__.port
        self.__class__.port += 1

        proc = Popen(["python", "echo.py", str(port)])
        try:
            callback(port)
        finally:
            self.kill_proc(proc)

    def test_TcpSocket0(self):
        def test(port):
            self._test("""
import socket
enable_gc_stress()
sock = socket.TcpSocket.new(\"127.0.0.1\", %(port)u)
try
  sock.send(\"foo\")
  print(sock.recv(1))
  print(sock.recv(1))
  print(sock.recv(1))
finally
  sock.close()
end
""" % locals(), "b\"\\x66\"b\"\\x6f\"b\"\\x6f\"")
        self.do_test(test)

    def test_TcpSocket10(self):
        def test(port):
            self._test("""
import socket
enable_gc_stress()
sock = socket.TcpSocket.new(\"localhost\", %(port)u)
try
  sock.send(\"foo\")
  print(sock.recv(1))
  print(sock.recv(1))
  print(sock.recv(1))
finally
  sock.close()
end
""" % locals(), "b\"\\x66\"b\"\\x6f\"b\"\\x6f\"")
        self.do_test(test)

# vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
