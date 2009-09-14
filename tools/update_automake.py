#! python
# -*- coding: utf-8 -*-

from os import listdir
from os.path import isfile, join, splitext
from sys import argv

class AutomakeUpdater(object):

    exts = [".c", ".y", ]

    def listfiles(self, dirpath, dirname):
        retval = []
        for filename in listdir(dirpath):
            if filename.startswith("."):
                continue
            if filename == "parser.c":
                continue

            path = join(dirpath, filename)
            if isfile(path):
                head, ext = splitext(filename)
                if ext in self.exts:
                    path = join(dirname, filename)
                    if path not in retval:
                        retval.append(path)
                elif ext == ".tmpl":
                    ext2 = splitext(head)[1]
                    if ext2 in self.exts:
                        path = join(dirname, head)
                        if path not in retval:
                            retval.append(join(dirname, head))
        return retval

    def do(self, dirpath):
        files = " \\\n\t".join(sorted(self.listfiles(dirpath, "")))

        template = join(dirpath, "Makefile.am.tmpl")
        dest = join(dirpath, "Makefile.am")
        with open(template) as tmpl:
            with open(dest, "w") as am:
                for line in tmpl:
                    am.write(line.replace("@SOURCES@", files))

if __name__ == "__main__":
    AutomakeUpdater().do(argv[1])

# vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
