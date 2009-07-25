# -*- coding: utf-8 -*-

def is_32bit():
    try:
        return 1 >> (1 << 31 - 1) == 0
    except OverflowError:
        return False

# vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
