# -*- coding: utf-8 -*-

from os import listdir
from os.path import exists, join, splitext
from re import compile, sub
from shutil import copy, move

exts = [".c", ".h", ".y", ".def", ]

def get_indent(line):
    s = []
    for c in line:
        if c != " ":
            return "".join(s)
        s.append(c)

def parse_args(line):
    args = []

    level = 0
    s = []
    for c in line:
        if c == "(":
            if 0 < level:
                s.append(c)
            level += 1
        elif level == 1:
            if c == ")":
                args.append("".join(s))
                return args
            elif c == ",":
                args.append("".join(s))
                s = []
            else:
                s.append(c)
        elif 0 < level:
            if c == ")":
                level -= 1
            s.append(c)

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
                        if line.find("MODIFY") < 0:
                            out.write(line)
                        else:
                            indent = get_indent(line)
                            args = parse_args(line)
                            print "%s, %d, \"%s\"" % (line, len(indent), args[1])
                            out.write("%s%s = %s;\n" % (indent, args[1].strip(), args[2].strip()))
            move(tmp, path)

def main():
    for dir in ["src", "src/gc", "include/yog", "tests", "tests/encoding"]:
        replace_for_dir(dir)

if __name__ == "__main__":
    main()

# vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
