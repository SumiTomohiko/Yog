#include "yog/config.h"
#include <errno.h>
#include "zip.h"
#include "yog/array.h"
#include "yog/error.h"
#include "yog/eval.h"
#include "yog/get_args.h"
#include "yog/package.h"
#include "yog/string.h"
#include "yog/vm.h"
#include "yog/yog.h"

struct Package {
    struct YogPackage base;
    YogVal eZipError;
};

typedef struct Package Package;

#define TYPE_ZIP_PKG    ((type_t)Package_new)

static void
Package_keep_children(YogEnv* env, void* ptr, ObjectKeeper keeper, void* heap)
{
    YogPackage_keep_children(env, ptr, keeper, heap);

    Package* pkg = (Package*)ptr;
#define KEEP(member)    YogGC_keep(env, &pkg->member, keeper, heap)
    KEEP(eZipError);
#undef KEEP
}

static YogVal
Package_new(YogEnv* env)
{
    SAVE_LOCALS(env);
    YogVal pkg = YUNDEF;
    PUSH_LOCAL(env, pkg);

    pkg = ALLOC_OBJ(env, Package_keep_children, NULL, Package);
    YogObj_init(env, pkg, TYPE_ZIP_PKG, FLAG_PKG, env->vm->cPackage);
    PTR_AS(Package, pkg)->eZipError = YUNDEF;
    YogPackage_init(env, pkg, TYPE_ZIP_PKG);

    RETURN(env, pkg);
}

static void
raise_ZipError(YogEnv* env, YogVal pkg, const char* msg)
{
    SAVE_ARG(env, pkg);
    YogVal eZipError = YUNDEF;
    YogVal e = YUNDEF;
    YogVal s = YUNDEF;
    PUSH_LOCALS3(env, eZipError, e, s);
    if (!IS_PTR(pkg) || (BASIC_OBJ_TYPE(pkg) != TYPE_ZIP_PKG)) {
        YogError_raise_TypeError(env, "package type must be TYPE_ZIP_PKG");
    }

    eZipError = PTR_AS(Package, pkg)->eZipError;
    if (msg != NULL) {
        s = YogString_from_str(env, msg);
        e = YogEval_call_method1(env, eZipError, "new", s);
    }
    else {
        e = YogEval_call_method0(env, eZipError, "new");
    }
    YogError_raise(env, e);
    /* NOTREACHED */

    RETURN_VOID(env);
}

static YogVal
compress(YogEnv* env, YogVal self, YogVal pkg, YogVal args, YogVal kw, YogVal block)
{
    SAVE_ARGS5(env, self, pkg, args, kw, block);
    YogVal path = YUNDEF;
    YogVal files = YUNDEF;
    YogVal name = YUNDEF;
    PUSH_LOCALS3(env, path, files, name);
    YogCArg params[] = { { "path", &path }, { "*", &files }, { NULL, NULL } };
    YogGetArgs_parse_args(env, "compress", params, args, kw);
    if (!IS_PTR(path) || (BASIC_OBJ_TYPE(path) != TYPE_STRING)) {
        YogError_raise_TypeError(env, "path must be String");
    }

    int error;
    struct zip* archive = zip_open(STRING_CSTR(path), ZIP_CREATE, &error);
    if (archive == NULL) {
        char buf[256];
        zip_error_to_str(buf, array_sizeof(buf), error, errno);
        raise_ZipError(env, pkg, buf);
    }
    int n = zip_get_num_files(archive);
    int i;
    for (i = 0; i < n; i++) {
        if (zip_delete(archive, i) != 0) {
            raise_ZipError(env, pkg, zip_strerror(archive));
        }
    }
    uint_t size = YogArray_size(env, files);
    uint_t j;
    for (j = 0; j < size; j++) {
        name = YogArray_at(env, files, j);
        if (!IS_PTR(name) || (BASIC_OBJ_TYPE(name) != TYPE_STRING)) {
            YogError_raise_TypeError(env, "filename must be String");
        }
        const char* fname = STRING_CSTR(name);
        struct zip_source* source = zip_source_file(archive, fname, 0, 0);
        if (source == NULL) {
            raise_ZipError(env, pkg, zip_strerror(archive));
        }
        if (zip_add(archive, fname, source) < 0) {
            raise_ZipError(env, pkg, zip_strerror(archive));
        }
    }
    if (zip_close(archive) != 0) {
        raise_ZipError(env, pkg, zip_strerror(archive));
    }

    RETURN(env, YNIL);
}

static YogVal
decompress(YogEnv* env, YogVal self, YogVal pkg, YogVal args, YogVal kw, YogVal block)
{
    SAVE_ARGS5(env, self, pkg, args, kw, block);
    RETURN(env, YNIL);
}

YogVal
YogInit_zip(YogEnv* env)
{
    SAVE_LOCALS(env);
    YogVal pkg = YUNDEF;
    YogVal eZipError = YUNDEF;
    PUSH_LOCALS2(env, pkg, eZipError);

    pkg = Package_new(env);

    eZipError = YogClass_new(env, "ZipError", env->vm->eException);
    PTR_AS(Package, pkg)->eZipError = eZipError;

    YogPackage_define_function(env, pkg, "compress", compress);
    YogPackage_define_function(env, pkg, "decompress", decompress);

    RETURN(env, pkg);
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
