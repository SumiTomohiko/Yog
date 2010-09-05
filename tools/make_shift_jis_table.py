# -*- coding: utf-8 -*-

from __future__ import print_function
from os.path import dirname, join
from encoding import read_entries, write_table, write_yog2bin_table, write_yog2bin_table2

def iterate_hankaku():
    for c in range(0xa1, 0xdf + 1):
        yield (c, )

def iterate_higher_byte():
    for c in range(0x81, 0x9f + 1):
        yield c
    for c in range(0xe0, 0xef + 1):
        yield c

def iterate_lower_byte():
    for c in range(0x40, 0x7e + 1):
        yield c
    for c in range(0x80, 0xfc + 1):
        yield c

def iterate_zenkaku():
    for c1 in iterate_higher_byte():
        for c2 in iterate_lower_byte():
            yield c1, c2

src_dir = join(dirname(__file__), "..", "src")
bin2entry, u2entry = read_entries(join(src_dir, "sjis-0213-2004-std.txt"))
write_table(join(src_dir, "shift_jis2yog_hankaku.inc"), iterate_hankaku(), bin2entry)
write_table(join(src_dir, "shift_jis2yog_zenkaku.inc"), iterate_zenkaku(), bin2entry)

def iterate_unicode1():
    for u in range(0xa0, 0x1ff + 1):
        yield u

def iterate_unicode2():
    for u in range(0x250, 0x451 + 1):
        yield u

def iterate_unicode3():
    for u in range(0x1e3e, 0x9fa2 + 1):
        yield u
    for u in range(0xf91d, 0xff9f + 1):
        yield u
    for u in range(0x2000b, 0x2a6b2 + 1):
        yield u

def write_yog2shift_jis_table(index, unicode_collection):
    path = join(src_dir, "yog2shift_jis%d.inc" % (index, ))
    write_yog2bin_table(path, unicode_collection, u2entry)

write_yog2shift_jis_table(1, iterate_unicode1())
write_yog2shift_jis_table(2, iterate_unicode2())

path = join(src_dir, "yog2shift_jis3.inc")
write_yog2bin_table2(path, iterate_unicode3(), u2entry, "YOG2SHIFT_JIS3_NUM")

# vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
