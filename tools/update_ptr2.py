# -*- coding: utf-8 -*-

from os import listdir
from os.path import exists, join, splitext
from re import sub
from shutil import copyfile

def files():
    for d in [join("include", "yog"), "src", join("ext", "concurrent"),
            join("ext", "socket"), join("ext", "zlib"), join("ext", "zip"),
            join("ext", "yaml")]:
        for f in listdir(d):
            if splitext(f)[1] not in [".h", ".c", ".y"]:
                continue
            yield join(d, f)

def main():
    for path in files():
        backup = path + "~"
        if not exists(backup):
            copyfile(path, backup)
        with open(backup) as fpin:
            with open(path, "w") as fpout:
                for line in fpin:
                    line = sub(r"YogGC_keep\(env, &(?P<obj>\w+)->(?P<member>[\w\[\]\.]+), keeper, (?P<heap>\w+)\)", r"YogGC_KEEP(env, \g<obj>, \g<member>, keeper, \g<heap>)", line)
                    fpout.write(line)

main()

# vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
