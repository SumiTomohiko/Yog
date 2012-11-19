# -*- coding: utf-8 -*-

from socket import AF_INET, SOCK_STREAM, error, socket
from subprocess import Popen
from time import sleep
from testcase import TestLib

class TestSocket(TestLib):

    port = 1092
    options = []

    def try_to_connect(self, port):
        sock = socket(AF_INET, SOCK_STREAM)
        try:
            sock.connect(("127.0.0.1", port))
        except error:
            return False
        sock.close()
        return True

    def wait_server_start(self, port):
        n = 0
        max = 4
        while (n < max) and (not self.try_to_connect(port)):
            n += 1
            sleep(1)
        assert n != max

    def do_test(self, callback):
        port = self.__class__.port
        self.__class__.port += 1

        proc = Popen(["python", "echo.py", str(port)])
        try:
            self.wait_server_start(port)
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
  bin = \"foo\".to_cstr(ENCODINGS[\"ascii\"])
  data = bin.slice(0, bin.size - 1)
  sock.send(data)
  print(sock.recv(1))
  print(sock.recv(1))
  print(sock.recv(1))
finally
  sock.close()
end
""" % locals(), "foo")
        self.do_test(test)

    def test_TcpSocket10(self):
        def test(port):
            self._test("""
import socket
enable_gc_stress()
sock = socket.TcpSocket.new(\"localhost\", %(port)u)
try
  bin = \"foo\".to_cstr(ENCODINGS[\"ascii\"])
  data = bin.slice(0, bin.size - 1)
  sock.send(data)
  print(sock.recv(1))
  print(sock.recv(1))
  print(sock.recv(1))
finally
  sock.close()
end
""" % locals(), "foo")
        self.do_test(test)

# vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
