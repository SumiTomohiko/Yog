# -*- coding: utf-8 -*-

from os import listdir
from os.path import exists, join, splitext
from re import compile
from shutil import copy, move

def main():
    dirname = "src"
    for name in listdir(dirname):
        ext = splitext(name)[1]
        if ext in (".c", ".y"):
            path = join(dirname, name)
            orig = path + ".orig"
            if not exists(orig):
                copy(path, orig)

            with open(orig) as in_:
                tmp = path + ".tmp"
                with open(tmp, "w") as out:
                    for line in in_:
                        regex = compile(r"^(.*)(ALLOC_OBJ\w*)\(([^,]+),\s*([^,]+),\s+([^,]+)\)(.*)")
                        m = regex.match(line)
                        if m is not None:
                            s = "%s%s(%s, %s, NULL, %s)%s\n" % (m.group(1), m.group(2), m.group(3), m.group(4), m.group(5), m.group(6))
                            out.write(s)
                            continue

                        regex = compile(r"^(.*)(ALLOC_OBJ\w*)\(([^,]+),\s*([^,]+),\s+([^,]+),\s+([^,]+),\s+([^,]+)\)(.*)")
                        m = regex.match(line)
                        if m is not None:
                            s = "%s%s(%s, %s, NULL, %s, %s, %s)%s\n" % (m.group(1), m.group(2), m.group(3), m.group(4), m.group(5), m.group(6), m.group(7), m.group(8))
                            out.write(s)
                            continue

                        out.write(line)
            move(tmp, path)

if __name__ == "__main__":
    main()

# vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
