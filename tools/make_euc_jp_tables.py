# -*- coding: utf-8 -*-

from __future__ import print_function
from os.path import dirname, join
from encoding import read_entries, write_table, write_yog2bin_table, write_yog2bin_table2

def iterate_euc_jp_range():
    for b in range(0xa1, 0xfe + 1):
        yield b

def iterate_hankaku():
    for b in iterate_euc_jp_range():
        yield 0x8e, b

def iterate_zenkaku():
    for b1 in iterate_euc_jp_range():
        for b2 in iterate_euc_jp_range():
            yield b1, b2

def iterate_3bytes():
    for b1 in iterate_euc_jp_range():
        for b2 in iterate_euc_jp_range():
            yield 0x8f, b1, b2

src_dir = join(dirname(__file__), "..", "src")
bin2entry, u2entry = read_entries(join(src_dir, "euc-jis-2004-std.txt"))
write_table(join(src_dir, "euc_jp2yog_zenkaku.inc"), iterate_zenkaku(), bin2entry)
write_table(join(src_dir, "euc_jp2yog_hankaku.inc"), iterate_hankaku(), bin2entry)
write_table(join(src_dir, "euc_jp2yog_3bytes.inc"), iterate_3bytes(), bin2entry)

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

def write_yog2euc_jp_table(index, unicode_collection):
    path = join(src_dir, "yog2euc_jp%d.inc" % (index, ))
    write_yog2bin_table(path, unicode_collection, u2entry)

write_yog2euc_jp_table(1, iterate_unicode1())
write_yog2euc_jp_table(2, iterate_unicode2())

path = join(src_dir, "yog2euc_jp3.inc")
write_yog2bin_table2(path, iterate_unicode3(), u2entry, "YOG2EUC_JP3_NUM")

# vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
