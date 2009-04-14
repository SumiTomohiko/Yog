# -*- coding: utf-8 -*-

from os import listdir
from os.path import exists, join, splitext
from re import compile
from shutil import copy, move

def main():
    dirname = "src"
    for name in listdir(dirname):
        ext = splitext(name)[1]
        if ext in (".c", ".y", ".lt"):
            path = join(dirname, name)
            orig = path + ".orig"
            if not exists(orig):
                copy(path, orig)

            with open(orig) as in_:
                tmp = path + ".tmp"
                with open(tmp, "w") as out:
                    for line in in_:
                        regex = compile(r"^(?P<head>.*)\bOBJ_AS\((?P<type>\w+),\s*(?P<var>\w+)\)(?P<tail>.*)$")
                        m = regex.match(line)
                        if m is not None:
                            s = "%(head)sPTR_AS(%(type)s, %(var)s)%(tail)s\n" % { "head": m.group("head"), "type": m.group("type"), "var": m.group("var"), "tail": m.group("tail") }
                            out.write(s)
                            continue

                        out.write(line)
            move(tmp, path)

if __name__ == "__main__":
    main()

# vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
