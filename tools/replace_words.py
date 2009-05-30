# -*- coding: utf-8 -*-

from os import listdir
from os.path import exists, join, splitext
from re import compile
from shutil import copy, move

exts = [".h", ".c", ".y", ".lt", ".py"]

def replace_for_dir(dirname):
    for name in listdir(dirname):
        ext = splitext(name)[1]
        if ext in exts:
            path = join(dirname, name)
            orig = path + ".orig"
            if not exists(orig):
                copy(path, orig)

            with open(orig) as in_:
                tmp = path + ".tmp"
                with open(tmp, "w") as out:
                    for line in in_:
                        out.write(line.replace("from tests", "from testcase"))
            move(tmp, path)

def main():
    for dir in ["src", "src/gc", "include/yog", "tests", "tests/encoding"]:
        replace_for_dir(dir)

if __name__ == "__main__":
    main()

# vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
