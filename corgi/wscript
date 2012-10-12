
from re import search

def read_version():
    with open("README.rst") as fp:
        for line in fp:
            m = search(r"\d+\.\d+\.\d+(?:[A-Za-z]+\d*)?$", line)
            if m is None:
                continue
            return m.group()
    raise Exception("Version number not found")

APPNAME = "corgi"
VERSION = read_version()

def options(ctx):
    ctx.load("compiler_c")

def add_config_prefix(name):
    return "CORGI_" + name

def check_sizeof(ctx, t, name):
    ctx.check_cc(
            fragment="""\
#include <stdio.h>
int
main(int argc, const char* argv[])
{{
    printf(\"%zu\", sizeof({t}));
    return 0;
}}
""".format(**locals()),
            define_name=add_config_prefix(name),
            execute=True,
            define_ret=True,
            quote=False,
            msg="Checking sizeof {t}".format(**locals()))

def check_header(ctx, name):
    define_name = add_config_prefix(name.upper().replace(".", "_"))
    ctx.check(header_name=name, define_name=define_name, mandatory=False)

def configure(ctx):
    options(ctx)
    check_header(ctx, "alloc.h")
    for t, name in [
            ["int", "SIZEOF_INT"],
            ["long", "SIZEOF_LONG"],
            ["long long", "SIZEOF_LONG_LONG"],
            ["void*", "SIZEOF_VOIDP"]]:
        check_sizeof(ctx, t, name)
    ctx.define(add_config_prefix("PACKAGE_VERSION"), VERSION)
    ctx.write_config_header("include/corgi/config.h")

def build(ctx):
    ctx.recurse("src")

def set_algo(ctx):
    ctx.algo = "tar.xz"

def dist(ctx):
    set_algo(ctx)
    ctx.excl = [".*", "build"]

def distcheck(ctx):
    set_algo(ctx)

# vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4 filetype=python
