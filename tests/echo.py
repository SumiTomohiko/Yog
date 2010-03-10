# -*- coding: utf-8 -*-

from socket import AF_INET, SO_REUSEADDR, SOL_SOCKET, SOCK_STREAM, socket

sock = socket(AF_INET, SOCK_STREAM, 0)
sock.setsockopt(SOL_SOCKET, SO_REUSEADDR, 1)
sock.bind(("127.0.0.1", 42 * 26))
try:
    sock.listen(1)
    while True:
        conn, addr = sock.accept()
        try:
            while True:
                conn.send(conn.recv(1))
        finally:
            conn.close()
finally:
    sock.close()

# vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
