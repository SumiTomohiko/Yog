#! python3.1
# -*- coding: utf-8 -*-

from os import listdir
from os.path import exists, join, splitext
from re import match, sub
from shutil import copyfile

class Struct:
    def __init__(self, name=""):
        self.name = name
        self.members = {}

    def __repr__(self):
        return "<Struct %s>" % (", ".join([self.name] + [repr(self.members[key]) if self.members[key] is not None else repr(key) for key in self.members]))

def log(s):
    print(s)

def parse_structs(path):
    log("processing %s..." % (path, ))

    stack = []
    structs = {}
    with open(path) as fp:
        for line in fp:
            line = line.strip()
            if line == "};":
                if len(stack) < 1:
                    continue
                struct = stack.pop()
                structs[struct.name] = struct
                continue
            m = match(r"YogVal (?P<name>\w+)(\[\d+\])?;", line)
            if m is not None:
                if len(stack) < 1:
                    continue
                name = m.group("name")
                log("    member %s found" % (name, ))
                stack[-1].members[name] = None
                continue
            m = match(r"} (?P<name>\w+);", line)
            if m is not None:
                if len(stack) < 1:
                    continue
                name = m.group("name")
                stack[-1].name = name
                struct = stack.pop()
                if len(stack) < 1:
                    continue
                stack[-1].members[name] = struct
                continue
            m = match(r"(union|struct) {", line)
            if m is not None:
                log("    internal structure found")
                stack.append(Struct())
                continue
            columns = line.split()
            if len(columns) != 3:
                continue
            if (columns[0] != "struct") or (columns[2] != "{"):
                continue
            name = columns[1]
            log("  struct %s found" % (name, ))
            stack.append(Struct(name))

    return structs

def files(exts):
    dirs = [join("include", "yog"), "src", join("ext", "concurrent"),
            join("ext", "socket"), join("ext", "zlib"), join("ext", "zip"),
            join("ext", "yaml")]
    for d in dirs:
        for f in listdir(d):
            if splitext(f)[1] not in exts:
                continue
            yield join(d, f)

def parse_srcs():
    structs = {}
    for path in files([".h", ".c", ".y"]):
        dict = parse_structs(path)
        for key in dict:
            if key in structs:
                structs[key].members.update(dict[key].members)
                continue
            structs[key] = dict[key]
    return structs

def backup(path):
    backup_path = path + "~"
    if exists(backup_path):
        return backup_path
    copyfile(path, backup_path)
    return backup_path

def conv_macro_name(name):
    s = []
    for c in name:
        if c.isupper() and (0 < len(s)):
            s.append("_")
        s.append(c.upper())
    return "".join(s)

def find_struct(structs, name):
    for key in structs:
        if conv_macro_name(key) == name:
            return structs[key]
    for key in structs:
        prefix = "Yog"
        s = key[len(prefix):] if key.startswith(prefix) else key
        if conv_macro_name(s) == name:
            return structs[key]

def member_exists(struct, name):
    name = sub(r"\[.*\]", "", name)
    for s in name.split("."):
        if s not in struct.members:
            return False
        if struct.members[s] is None:
            return True
        struct = struct.members[s]
    return False

def rewrite_srcs(structs):
    for path in files([".c", ".y", ".def"]):
        backup_path = backup(path)
        with open(path, "w") as fpout:
            with open(backup_path) as fpin:
                for line in fpin:
                    m = match(r"(?P<indent>\s*)PTR_AS\((?P<type>\w+), (?P<var>[->\w\(\)]+)\)->(?P<name>.+) = (?P<val>.+);(?P<tail>.*)$", line)
                    if m is not None:
                        name = m.group("name")
                        if not member_exists(structs[m.group("type")], name):
                            fpout.write(line)
                            continue
                        if m.group("val") in ["YNIL", "YUNDEF"]:
                            fpout.write(line)
                            continue
                        fpout.write("%sYogGC_UPDATE_PTR(env, PTR_AS(%s, %s), %s, %s);%s\n" % (m.group("indent"), m.group("type"), m.group("var"), name, m.group("val"), m.group("tail")))
                        continue

                    m = match(r"(?P<indent>\s*)(?P<type>\w+)\((?P<var>\w+)\)->(?P<name>.+) = (?P<val>.+);(?P<tail>.*)$", line)
                    if m is not None:
                        struct = find_struct(structs, m.group("type"))
                        if struct is None:
                            fpout.write(line)
                            continue
                        name = m.group("name")
                        if not member_exists(struct, name):
                            fpout.write(line)
                            continue
                        if m.group("val") in ["YNIL", "YUNDEF"]:
                            fpout.write(line)
                            continue
                        fpout.write("%sYogGC_UPDATE_PTR(env, %s(%s), %s, %s);%s\n" % (m.group("indent"), m.group("type"), m.group("var"), name, m.group("val"), m.group("tail")))
                        continue

                    fpout.write(line)

def main():
    structs = parse_srcs()
    log("found structures...")
    for name in structs:
        log(structs[name])
    rewrite_srcs(structs)

main()

# vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
