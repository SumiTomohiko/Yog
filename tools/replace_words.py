# -*- coding: utf-8 -*-

from os import listdir
from os.path import join, splitext
from shutil import move

def main():
    dirname = "src"
    for name in listdir(dirname):
        ext = splitext(name)[1]
        if ext in (".c", ".y"):
            path = join(dirname, name)
            with open(path) as in_:
                tmp = path + ".tmp"
                with open(tmp, "w") as out:
                    for line in in_:
                        out.write(line.replace("Yog_assert", "YOG_ASSERT"))
            move(tmp, path)

if __name__ == "__main__":
    main()

# vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
