# -*- coding: utf-8 -*-

from sys import stdin

SEARCH_SOURCE = 1
READ_SOURCE = 2

status = SEARCH_SOURCE
source_id = 0

for line in stdin:
    if status == SEARCH_SOURCE:
        headline = "        self._test(\"\"\""
        if line.startswith(headline):
            source = [line[len(headline):]]
            status = READ_SOURCE
    else:
        endmarker = "\"\"\""
        index = line.find(endmarker)
        if index != -1:
            source.append(line[:index])
            with open("error_%(id)03d.yg" % { "id": source_id }, "w") as fp:
                fp.write("".join(source))
                from sys import stdout
                stdout.write("".join(source))
            source_id += 1
            status = SEARCH_SOURCE
        else:
            source.append(line)

# vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
