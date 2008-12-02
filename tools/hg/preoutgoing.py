# -*- coding: utf-8 -*-

from ConfigParser import ConfigParser
from subprocess import PIPE, Popen
from urllib import urlencode, urlopen

def hook(ui, repo, **kwargs):
    proc = Popen(["hg", "diff"], stdout=PIPE)
    proc.wait()
    stdout = proc.stdout.read()
    size = len(stdout)

    parser = ConfigParser()
    parser.read("outputz.ini")
    key = parser.get("outputz", "key")
    uri = parser.get("outputz", "uri")

    params = urlencode({ "key": key, "uri": uri, "size": str(size) })
    f = urlopen("http://outputz.com/api/post", params)
    if f.read() != "ok":
        raise Exception("can't post to outputz")

# vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
