#include "yog/config.h"
#include <errno.h>
#include <stdio.h>
#include "zlib.h"
#include "yog/binary.h"
#include "yog/error.h"
#include "yog/eval.h"
#include "yog/exception.h"
#include "yog/get_args.h"
#include "yog/object.h"
#include "yog/package.h"
#include "yog/string.h"
#include "yog/vm.h"
#include "yog/yog.h"

struct Package {
    struct YogPackage base;
    YogVal eZlibError;
};

typedef struct Package Package;

#define TYPE_ZLIB_PKG   ((type_t)Package_new)

static void
Package_keep_children(YogEnv* env, void* ptr, ObjectKeeper keeper, void* heap)
{
    YogPackage_keep_children(env, ptr, keeper, heap);

    Package* pkg = (Package*)ptr;
#define KEEP(member)    YogGC_keep(env, &pkg->member, keeper, heap)
    KEEP(eZlibError);
#undef KEEP
}

static YogVal
Package_new(YogEnv* env)
{
    SAVE_LOCALS(env);
    YogVal pkg = YUNDEF;
    PUSH_LOCAL(env, pkg);

    pkg = ALLOC_OBJ(env, Package_keep_children, NULL, Package);
    YogObj_init(env, pkg, TYPE_ZLIB_PKG, FLAG_PKG, env->vm->cPackage);
    PTR_AS(Package, pkg)->eZlibError = YUNDEF;
    YogPackage_init(env, pkg, TYPE_ZLIB_PKG);

    RETURN(env, pkg);
}

static void
raise_ZlibError(YogEnv* env, YogVal pkg, const char* msg)
{
    SAVE_ARG(env, pkg);
    YogVal eZlibError = YUNDEF;
    YogVal e = YUNDEF;
    YogVal s = YUNDEF;
    PUSH_LOCALS3(env, eZlibError, e, s);
    if (!IS_PTR(pkg) || (BASIC_OBJ_TYPE(pkg) != TYPE_ZLIB_PKG)) {
        YogError_raise_TypeError(env, "package type must be TYPE_ZLIB_PKG");
    }

    eZlibError = PTR_AS(Package, pkg)->eZlibError;
    if (msg != NULL) {
        s = YogString_from_str(env, msg);
        e = YogEval_call_method1(env, eZlibError, "new", s);
    }
    else {
        e = YogEval_call_method0(env, eZlibError, "new");
    }
    YogError_raise(env, e);
    /* NOTREACHED */

    RETURN_VOID(env);
}

static YogVal
decompress(YogEnv* env, YogVal self, YogVal pkg, YogVal args, YogVal kw, YogVal block)
{
    SAVE_ARGS5(env, self, pkg, args, kw, block);
    YogVal bytes = YUNDEF;
    YogVal bin = YUNDEF;
    PUSH_LOCALS2(env, bytes, bin);
    YogCArg params[] = { { "bytes", &bytes }, { NULL, NULL } };
    YogGetArgs_parse_args(env, "decompress", params, args, kw);
    if (!IS_PTR(bytes) || (BASIC_OBJ_TYPE(bytes) != TYPE_BINARY)) {
        YogError_raise_TypeError(env, "bytes must be Binary");
    }

    z_stream z;
    z.zalloc = Z_NULL;
    z.zfree = Z_NULL;
    z.opaque = Z_NULL;
    z.next_in = Z_NULL;
    z.avail_in = 0;
    if (inflateInit(&z) != Z_OK) {
        raise_ZlibError(env, pkg, z.msg);
    }

    bin = YogBinary_new(env);
    z.avail_in = YogBinary_size(env, bytes);
    while (0 < z.avail_in) {
#define BUF_SIZE    1024
        char buf[BUF_SIZE];
        uint_t size = YogBinary_size(env, bytes);
        z.next_in = (Bytef*)&BINARY_CSTR(bytes)[size - z.avail_in];
        z.next_out = (Bytef*)buf;
        z.avail_out = BUF_SIZE;
        int retval = inflate(&z, Z_NO_FLUSH);
        if (retval == Z_STREAM_END) {
            YogBinary_add(env, bin, buf, BUF_SIZE - z.avail_out);
            break;
        }
        if (retval != Z_OK) {
            inflateEnd(&z);
            raise_ZlibError(env, pkg, z.msg);
        }
        YogBinary_add(env, bin, buf, BUF_SIZE - z.avail_out);
#undef BUF_SIZE
    }

    if (inflateEnd(&z) != Z_OK) {
        raise_ZlibError(env, pkg, z.msg);
    }

    RETURN(env, bin);
}

static YogVal
compress_(YogEnv* env, YogVal self, YogVal pkg, YogVal args, YogVal kw, YogVal block)
{
    SAVE_ARGS5(env, self, pkg, args, kw, block);
    YogVal s = YUNDEF;
    YogVal bin = YUNDEF;
    PUSH_LOCALS2(env, s, bin);
    YogCArg params[] = { { "s", &s }, { NULL, NULL } };
    YogGetArgs_parse_args(env, "compress", params, args, kw);
    if (!IS_PTR(s) || (BASIC_OBJ_TYPE(s) != TYPE_STRING)) {
        YogError_raise_TypeError(env, "s must be String");
    }

    bin = YogBinary_new(env);
    z_stream z;
    z.zalloc = Z_NULL;
    z.zfree = Z_NULL;
    z.opaque = Z_NULL;
    if (deflateInit(&z, Z_DEFAULT_COMPRESSION) != Z_OK) {
        raise_ZlibError(env, pkg, z.msg);
    }
    z.avail_in = YogString_size(env, s);
#define BUF_SIZE    1024
    while (0 < z.avail_in) {
        char out_buf[BUF_SIZE];
        z.next_in = (Bytef*)STRING_CSTR(s);
        z.next_out = (Bytef*)out_buf;
        z.avail_out = BUF_SIZE;
        if (deflate(&z, Z_NO_FLUSH) != Z_OK) {
            deflateEnd(&z);
            raise_ZlibError(env, pkg, z.msg);
        }
        YogBinary_add(env, bin, out_buf, BUF_SIZE - z.avail_out);
    }
    while (1) {
        char out_buf[BUF_SIZE];
        z.next_out = (Bytef*)out_buf;
        z.avail_out = BUF_SIZE;
        int retval = deflate(&z, Z_FINISH);
        if (retval == Z_STREAM_END) {
            YogBinary_add(env, bin, out_buf, BUF_SIZE - z.avail_out);
            break;
        }
        if (retval != Z_OK) {
            deflateEnd(&z);
            raise_ZlibError(env, pkg, z.msg);
        }
        YogBinary_add(env, bin, out_buf, BUF_SIZE - z.avail_out);
    }
#undef BUF_SIZE

    if (deflateEnd(&z) != Z_OK) {
        raise_ZlibError(env, pkg, z.msg);
    }

    RETURN(env, bin);
}

YogVal
YogInit_zlib(YogEnv* env)
{
    SAVE_LOCALS(env);
    YogVal pkg = YUNDEF;
    YogVal eZlibError = YUNDEF;
    PUSH_LOCALS2(env, pkg, eZlibError);

    pkg = Package_new(env);

    eZlibError = YogClass_new(env, "ZlibError", env->vm->eException);
    PTR_AS(Package, pkg)->eZlibError = eZlibError;

    YogPackage_define_function(env, pkg, "compress", compress_);
    YogPackage_define_function(env, pkg, "decompress", decompress);

    RETURN(env, pkg);
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
