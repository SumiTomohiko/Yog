# -*- coding: utf-8 -*-

from os.path import dirname, join
from re import IGNORECASE, compile
from sys import argv
from uuid import uuid4

def make_wxs_name(ver):
    return "Yog-%(ver)s.wxs" % locals()

def do(dir, ver_old, ver_new):
    guid_re = compile(r"\w+-\w+-\w+-\w+-\w+", IGNORECASE)
    with open(make_wxs_name(ver_old)) as fp_in:
        with open(make_wxs_name(ver_new), "w") as fp_out:
            for line in fp_in:
                m = guid_re.search(line)
                if m is None:
                    l = line
                else:
                    l = line[:m.start()] + str(uuid4()) + line[m.end():]
                l = l.replace(ver_old, ver_new)
                fp_out.write(l)

if __name__ == "__main__":
    dir = join(dirname(argv[0]), "..")
    ver_old = argv[1]
    ver_new = argv[2]
    do(dir, ver_old, ver_new)

# vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
