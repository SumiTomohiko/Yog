# -*- coding: utf-8 -*-

from os import environ
from subprocess import PIPE, Popen
from sys import exit

args = [environ["CORGI"], "match", "[ \\t\\n\\r]", "\r"]
proc = Popen(args, stdout=PIPE)
stdout = proc.stdout.read().decode("UTF-8")
proc.wait()
if stdout != "\r":
    exit(1)
exit(0)

# vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
