# -*- coding: utf-8 -*-

from os import walk
from os.path import exists, join
from re import match
from shutil import copyfile

def main(path):
    for dirpath, dirnames, filenames in walk(path):
        for filename in filenames:
            exts = [".h", ".c", ".y", ".lt"]
            has_ext = False
            for ext in exts:
                if filename.endswith(ext):
                    has_ext = True
                    break
            if has_ext is False:
                continue

            filepath = join(dirpath, filename)
            backup_file = filepath + ".orig"
            if not exists(backup_file):
                copyfile(filepath, backup_file)
            with open(filepath, "w") as outfp:
                with open(backup_file) as infp:
                    for line in infp:
                        m = match(r"#ifndef (?P<name>\w+)", line)
                        if m is not None:
                            outfp.write("#if !defined(%(name)s)\n" % { "name": m.group("name") })
                            continue
                        m = match(r"#ifdef (?P<name>\w+)", line)
                        if m is not None:
                            outfp.write("#if defined(%(name)s)\n" % { "name": m.group("name") })
                            continue
                        outfp.write(line)

if __name__ == "__main__":
    main("src")
    main("include")

# vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
