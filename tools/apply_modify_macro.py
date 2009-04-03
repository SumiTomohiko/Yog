# -*- coding: utf-8 -*-

from os import walk
from os.path import exists, join
from re import match
from shutil import copyfile

def listfiles(path, ext):
    for dirpath, dirnames, filenames in walk(path):
        for filename in filenames:
            if filename.endswith(ext):
                yield join(dirpath, filename)

class Struct(object):

    def __init__(self, name):
        self.name = name
        self.members = {}

def parse_struct(fp, name):
    st = Struct(name)
    for line in fp:
        #print "line", line, 
        m = match(r"^\s*union \{$", line)
        if m is not None:
            s = parse_struct(fp, None)
            st.members[s.name] = s
            continue
        m = match(r"^\s*struct \{$", line)
        if m is not None:
            s = parse_struct(fp, None)
            st.members[s.name] = s
            continue
        m = match(r"^\s*struct (?P<type>\w+\*?) (?P<name>\w+);\s*$", line)
        if m is not None:
            #print name, m.group("type"), m.group("name")
            st.members[m.group("name")] = m.group("type")
            continue
        m = match(r"^\s*} (?P<name>\w+);$", line)
        if m is not None:
            st.name = m.group("name")
            return st
        m = match(r"^\s*};$", line)
        if m is not None:
            break
    #print "parse_struct", name, st.name
    return st

def parse_source(file):
    decl = {}
    macro = {}
    with open(file) as fp:
        for line in fp:
            m = match(r"^\s*struct (?P<name>\w+) \{$", line)
            if m is not None:
                st = parse_struct(fp, m.group("name"))
                decl[st.name] = st
                continue
            m = match(r"^#\s*define\s*(?P<name>\w+)\(.*\)\s+(PTR_AS|OBJ_AS)\((?P<type>\w+), .+\)$", line)
            if m is not None:
                #print m.group("name"), m.group("type")
                macro[m.group("name")] = m.group("type")
                continue
    return decl, macro

def list_decl_files():
    path = "src"
    for file in listfiles(path, ".c"):
        yield file
    for file in listfiles(path, ".y"):
        yield file
    for file in listfiles("include", ".h"):
        yield file

def get_decl():
    decl = {}
    macro = {}
    for file in list_decl_files():
        d, m = parse_source(file)
        decl.update(d)
        macro.update(m)
    return decl, macro

def get_type(decl, type, member):
    #print decl.get(type, None), type, member
    try:
        st = decl[type]
    except KeyError:
        return None
    #print type
    for m in member.split("."):
        #print m, st.members
        try:
            st = st.members[m]
        except KeyError:
            return None
    return st

def get_macro_type(macro, decl, name, member):
    try:
        type = macro[name]
    except KeyError:
        return None
    return get_type(decl, type, member)

def replace_file(file, decl):
    with open(file + ".orig") as infp:
        with open(file, "w") as outfp:
            for line in infp:
                m = match(r"^(?P<indent>\s*)(?P<macro>PTR_AS|OBJ_AS)\((?P<type>\w+), (?P<obj>\w+)\)->(?P<member>[\.\w]+) = (?P<val>.+);$", line)
                if m is not None:
                    val = m.group("val")
                    if (val != "YUNDEF") and (val != "YNIL") and (val != "NULL"):
                        type = get_type(decl, m.group("type"), m.group("member"))
                        if type is not None:
                            if isinstance(type, basestring):
                                if type.endswith("*") or (type == "YogVal"):
                                    outfp.write("%(indent)sMODIFY(env, %(macro)s(%(type)s, %(obj)s)->%(member)s, %(val)s);\n" % { "indent": m.group("indent"), "obj": m.group("obj"), "macro": m.group("macro"), "type": m.group("type"), "member": m.group("member"), "val": m.group("val") })
                                    continue
                m = match(r"^(?P<indent>\s*)(?P<macro>\w+)\((?P<arg>\w+)\)->(?P<member>[\w\.]+) = (?P<val>.+);$", line)
                if m is not None:
                    val = m.group("val")
                    if (val != "YUNDEF") and (val != "YNIL") and (val != "NULL"):
                        type = get_macro_type(macro, decl, m.group("macro"), m.group("member"))
                        #print line, type
                        if type is not None:
                            #print type
                            if isinstance(type, basestring):
                                #print type
                                if type.endswith("*") or (type == "YogVal"):
                                    #print type
                                    outfp.write("%(indent)sMODIFY(env, %(macro)s(%(arg)s)->%(member)s, %(val)s);\n" % { "indent": m.group("indent"), "macro": m.group("macro"), "arg": m.group("arg"), "member": m.group("member"), "val": m.group("val") })
                                    continue
                outfp.write(line)

def backup_file(file):
    backup = file + ".orig"
    if exists(backup):
        return
    copyfile(file, backup)

def list_source_files():
    path = "src"
    for file in listfiles(path, ".c"):
        yield file
    for file in listfiles(path, ".y"):
        yield file
    for file in listfiles(path, ".def"):
        yield file

def replace(decl, macro):
    for file in list_source_files():
        backup_file(file)
        replace_file(file, decl)

if __name__ == "__main__":
    decl, macro = get_decl()
    #print decl.get("YogNode")
    #print macro
    replace(decl, macro)

# vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
