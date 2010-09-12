# -*- coding: utf-8 -*-

from __future__ import print_function

class Entry(object):

    def __init__(self, lineno, bin, u):
        self.lineno = lineno
        self.bin = bin
        self.unicode = u

    def __repr__(self):
        lineno = self.lineno
        bin = ",".join(["0x%02x" % (c, ) for c in self.bin])
        u = "+".join(["%06x" % (u, ) for u in self.unicode])
        fmt = "<Entry lineno=%(lineno)d bin=[%(bin)s] unicode=U+%(u)s>"
        return fmt % locals()

def erase_comment(s):
    pos = s.find("#")
    if pos < 0:
        return s
    return s[:pos]

def hex2int(s):
    return int(s, 16)

def split_str_by_two(s):
    a = []
    i = 0
    while i < len(s):
        a.append(s[i:i + 2])
        i += 2
    return a

def make_bin_key(bin):
    return "".join(["0x%02x" % (c, ) for c in bin])

def read_entries(path):
    with open(path) as fp:
        bin2entry = {}
        u2entry = {}
        for i, line in enumerate(fp):
            s = erase_comment(line).strip()
            if s == "":
                continue
            cols = s.split()
            if len(cols) != 2:
                continue
            bin = [hex2int(c) for c in split_str_by_two(cols[0][2:])]
            u = [hex2int(c) for c in cols[1][2:].split("+")]
            entry = Entry(i + 1, bin, u)
            bin_key = make_bin_key(bin)
            u2entry[u[0]] = bin2entry[bin_key] = entry
        return bin2entry, u2entry

def write_bin2yog_entry(entry, fp):
    if entry is None:
        print("0x%06x, /* not found */" % (ord("?"), ), file=fp)
        return
    u = reduce(lambda x, y: (x << 16) + y, entry.unicode)
    s = "".join([unichr(c) for c in entry.unicode])
    bin = " ".join(["0x%02x" % (b, ) for b in entry.bin])
    lineno = entry.lineno
    fmt = "0x%(u)06x, /* %(s)s (%(bin)s) lineno: %(lineno)d */"
    print((fmt % locals()).encode("UTF-8"), file=fp)

def write_table(path, unicode_collection, bin2entry):
    with open(path, "w") as fp:
        for bin in unicode_collection:
            try:
                entry = bin2entry[make_bin_key(bin)]
            except KeyError:
                entry = None
            write_bin2yog_entry(entry, fp)

def join_bin(entry):
    return "".join(["\\x%02x" % (e, ) for e in entry.bin])

def write_yog2bin_table(path, unicode_collection, u2entry):
    with open(path, "w") as fp:
        for u in unicode_collection:
            try:
                entry = u2entry[u]
            except KeyError:
                s = "NULL, /* U+%06x */" % (u, )
            else:
                s = "\"%s\", /* U+%06x lineno: %d */" % (join_bin(entry), u, entry.lineno)
            print(s, file=fp)

def write_yog2bin_table2(path, unicode_collection, u2entry, num_name):
    with open(path, "w") as fp:
        n = 0
        for u in unicode_collection:
            n += 1
            try:
                entry = u2entry[u]
            except KeyError:
                continue
            fmt = "{ 0x%06x, \"%s\" }, /* lineno: %d */"
            print(fmt % (u, join_bin(entry), entry.lineno), file=fp)
        print("#define %s %d" % (num_name, n), file=fp)

# vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
