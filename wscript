
from re import search

def read_version():
    with open("README") as fp:
        for line in fp:
            m = search(r"\d+\.\d+\.\d+$", line)
            if m is None:
                continue
            return m.group()

APPNAME = "Yog"
VERSION = read_version()

def options(ctx):
    ctx.load("compiler_c")

def add_config_prefix(name):
    return "YOG_" + name

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

def check(ctx, name, **kw):
    define_name = add_config_prefix(ctx.have_define(name))
    ctx.check(define_name=define_name, mandatory=False, **kw)

def check_header(ctx, header):
    check(ctx, header, header_name=header)

def check_func(ctx, f, header):
    check(ctx, f, header_name=header, function_name=f)

def check_lib(ctx, lib):
    check(ctx, lib, lib=lib)

def exec_submodules(ctx, cmd):
    for mod in ["corgi", "libffi", "BigDigits"]:
        ctx.exec_command("cd {mod} && {cmd}".format(**locals()))

def check_errno(ctx, errno):
    ctx.check_cc(
            fragment="""\
#include <errno.h>
int
main(int argc, const char* argv[])
{{
    (void){errno};
    return 0;
}}
""".format(**locals()),
            define_name=add_config_prefix(ctx.have_define(errno)),
            quote=False,
            mandatory=False,
            msg="Checking errno {errno}".format(**locals()))

def define(ctx, key, value):
    ctx.define(add_config_prefix(key), value)

def configure(ctx):
    options(ctx)

    for header in [
            "inttypes.h", "limits.h", "stddef.h", "stdint.h", "stdlib.h",
            "string.h", "strings.h", "sys/time.h", "unistd.h", "malloc.h",
            "getopt.h", "float.h", "dlfcn.h", "sys/mman.h", "alloca.h",
            "arpa/inet.h", "netdb.h", "sys/socket.h", "direct.h"]:
        check_header(ctx, header)
    for t, name in [
            ["int", "SIZEOF_INT"],
            ["long", "SIZEOF_LONG"],
            ["long long", "SIZEOF_LONG_LONG"],
            ["void*", "SIZEOF_VOIDP"]]:
        check_sizeof(ctx, t, name)
    for errno in [
            "E2BIG", "EACCESS", "EADDINUSE", "EADDRNOTAVAIL", "EAFNOSUPPORT",
            "EAGAIN", "EALREADY", "EBADE", "EBADFD", "EBADF", "EBADMSG",
            "EBADRQC", "EBADR", "EBADSLT", "EBUSY", "ECANCELED", "ECHILD",
            "ECHRNG", "ECOMM", "ECONNABORTED", "ECONNREFUSED", "ECONNRESET",
            "EDEADLK", "EDEADLOCK", "EDESTADDRREQ", "EDOM", "EDQUOT", "EEXIST",
            "EFAULT", "EFBIG", "EHOSTDOWN", "EHOSTUNREACH", "EIDRM", "EILSEQ",
            "EINPROGRESS", "EINTR", "EINVAL", "EIO", "EISCONN", "EISDIR",
            "EISNAM", "EKEYEXPIRED", "EKEYREJECTED", "EKEYREVOKED", "EL2HLT",
            "EL2NSYNC", "EL3HLT", "EL3RST", "ELIBACC", "ELIBBAD", "ELIBEXEC",
            "ELIBMAX", "ELIBSCN", "ELOOP", "EMEDIUMTYPE", "EMFILE", "EMLINK",
            "EMSGSIZE", "EMULTIHOP", "ENAMETOOLONG", "ENETDOWN", "ENETRESET",
            "ENETUNREACH", "ENFILE", "ENOBUFS", "ENODATA", "ENODEV", "ENOENT",
            "ENOEXEC", "ENOKEY", "ENOLCK", "ENOLINK", "ENOMEDIUM", "ENOMEM",
            "ENOMSG", "ENONET", "ENOPKG", "ENOPROTOOPT", "ENOSPC", "ENOSR",
            "ENOSTR", "ENOSYS", "ENOTBLK", "ENOTCONN", "ENOTDIR", "ENOTEMPTY",
            "ENOTSOCK", "ENOTSUP", "ENOTTY", "ENOTUNIQ", "ENXIO", "EOPNOTSUPP",
            "EOVERFLOW", "EPERM", "EPFNOSUPPORT", "EPIPE", "EPROTONOSUPPORT",
            "EPROTOTYPE", "EPROTO", "ERANGE", "EREMCHG", "EREMOTEIO", "EREMOTE",
            "ERESTART", "EROFS", "ESHUTDOWN", "ESOCKTNOSUPPORT", "ESPIPE",
            "ESRCH", "ESTALE", "ESTRPIPE", "ETIMEDOUT", "ETIME", "ETXTBSY",
            "EUCLEAN", "EUNATCH", "EUSERS", "EWOULDBLOCK", "EXDEV", "EXFULL"]:
        check_errno(ctx, errno)
    for lib in ["pthread", "dl", "syck", "zip"]:
        check_lib(ctx, lib)
    ctx.env.append_value("LDFLAGS", ["-ldl"])
    for func, header in [("dlopen", "dlfcn.h")]:
        check_func(ctx, func, header)
    define(ctx, "PREFIX", ctx.env.PREFIX)
    define(ctx, "PACKAGE_VERSION", VERSION)
    ctx.write_config_header("include/yog/config.h")

    exec_submodules(ctx, "./configure")

def build(ctx):
    exec_submodules(ctx, "make")
    ctx.recurse("src")

def set_algo(ctx):
    ctx.algo = "tar.xz"

def dist(ctx):
    set_algo(ctx)
    ctx.excl = [".*", "build"]

def distcheck(ctx):
    set_algo(ctx)

# vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4 filetype=python
