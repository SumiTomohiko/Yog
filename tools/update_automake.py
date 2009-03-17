#! python
# -*- coding: utf-8 -*-

from os import listdir
from os.path import join, splitext

class AutomakeUpdater(object):

    exts = [".c", ".y", ]

    def do(self):
        files = []
        dirname = "src"
        for filename in sorted(listdir(dirname)):
            if splitext(filename)[1] in self.exts:
                files.append(filename)
        files = " \\\n\t".join(files)

        template = join(dirname, "Makefile.am.tmpl")
        dest = join(dirname, "Makefile.am")
        with open(template) as tmpl:
            with open(dest, "w") as am:
                for line in tmpl:
                    am.write(line.replace("@SOURCES@", files))

if __name__ == "__main__":
    AutomakeUpdater().do()

# vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
