#! python
# -*- coding: utf-8 -*-

from os import listdir
from os.path import join, splitext
from sys import argv

class AutomakeUpdater(object):

    exts = [".c", ".y", ]

    def do(self, dirpath):
        files = []
        for filename in sorted(listdir(dirpath)):
            if splitext(filename)[1] in self.exts:
                if filename != "parser.c":
                    files.append(filename)
        files = " \\\n\t".join(files)

        template = join(dirpath, "Makefile.am.tmpl")
        dest = join(dirpath, "Makefile.am")
        with open(template) as tmpl:
            with open(dest, "w") as am:
                for line in tmpl:
                    am.write(line.replace("@SOURCES@", files))

if __name__ == "__main__":
    AutomakeUpdater().do(argv[1])

# vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
