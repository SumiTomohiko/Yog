#! python
# -*- coding: utf-8 -*-

from os import listdir
from os.path import join, splitext

class AutomakeUpdater(object):

    exts = [".c", ".y", ]

    def do(self):
        files = []
        dirnames = ["src", "onig", join("onig", "enc")]
        for dirname in dirnames:
            for filename in sorted(listdir(dirname)):
                if filename not in ("koi8.c", "mktable.c", "test.c", "testc.c", "testu.c"):
                    if splitext(filename)[1] in self.exts:
                        files.append(join(dirname, filename))
        files = " \\\n\t".join(files)

        with open("Makefile.am.tmpl") as tmpl:
            with open("Makefile.am", "w") as am:
                for line in tmpl:
                    am.write(line.replace("@SOURCES@", files))

if __name__ == "__main__":
    AutomakeUpdater().do()

# vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
