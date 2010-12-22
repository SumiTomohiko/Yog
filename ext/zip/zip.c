#include "yog/config.h"
#include <errno.h>
#include <string.h>
#include "zip.h"
#include "yog/array.h"
#include "yog/binary.h"
#include "yog/error.h"
#include "yog/eval.h"
#include "yog/gc.h"
#include "yog/get_args.h"
#include "yog/handle.h"
#include "yog/misc.h"
#include "yog/package.h"
#include "yog/string.h"
#include "yog/sysdeps.h"
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
#define KEEP(member)    YogGC_KEEP(env, pkg, member, keeper, heap)
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
        s = YogString_from_string(env, msg);
        e = YogEval_call_method1(env, eZipError, "new", s);
    }
    else {
        e = YogEval_call_method0(env, eZipError, "new");
    }
    YogError_raise(env, e);
    /* NOTREACHED */

    RETURN_VOID(env);
}

static struct zip*
open_zip(YogEnv* env, YogVal pkg, YogVal path, int flags)
{
    SAVE_ARGS2(env, pkg, path);

    int error;
    YogVal bin = YogString_to_bin_in_default_encoding(env, VAL2HDL(env, path));
    struct zip* archive = zip_open(BINARY_CSTR(bin), flags, &error);
    if (archive == NULL) {
        char buf[256];
        zip_error_to_str(buf, array_sizeof(buf), error, errno);
        raise_ZipError(env, pkg, buf);
    }

    RETURN(env, archive);
}

static void
close_zip(YogEnv* env, YogVal pkg, struct zip* archive)
{
    SAVE_ARG(env, pkg);
    if (zip_close(archive) != 0) {
        raise_ZipError(env, pkg, zip_strerror(archive));
    }
    RETURN_VOID(env);
}

static YogVal
compress(YogEnv* env, YogVal self, YogVal pkg, YogVal args, YogVal kw, YogVal block)
{
    SAVE_ARGS5(env, self, pkg, args, kw, block);
    YogVal path = YUNDEF;
    YogVal files = YUNDEF;
    PUSH_LOCALS2(env, path, files);
    YogCArg params[] = { { "path", &path }, { "*", &files }, { NULL, NULL } };
    YogGetArgs_parse_args(env, "compress", params, args, kw);
    if (!IS_PTR(path) || (BASIC_OBJ_TYPE(path) != TYPE_STRING)) {
        YogError_raise_TypeError(env, "path must be String");
    }

    struct zip* archive = open_zip(env, pkg, path, ZIP_CREATE);
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
        YogHandle* name = VAL2HDL(env, YogArray_at(env, files, j));
        YogMisc_check_String(env, name, "Filename");
        YogVal bin = YogString_to_bin_in_default_encoding(env, name);
        const char* fname = BINARY_CSTR(bin);
        struct zip_source* source = zip_source_file(archive, fname, 0, 0);
        if (source == NULL) {
            raise_ZipError(env, pkg, zip_strerror(archive));
        }
        if (zip_add(archive, fname, source) < 0) {
            raise_ZipError(env, pkg, zip_strerror(archive));
        }
    }
    close_zip(env, pkg, archive);

    RETURN(env, YNIL);
}

static void
dirname(char* path)
{
    char* pc = strrchr(path, '/');
    if (pc == NULL) {
        *path = '\0';
        return;
    }
    *pc = '\0';
}

static void
make_dirs(YogEnv* env, const char* path)
{
    /**
     * TODO: Here is duplicated with builtins.make_dirs. To resolve this,
     * + make classes to represent struct zip etc.
     * + be open above classes to Yog script
     * + implement creating/extracting zips in Yog
     */
    char* dir = (char*)YogSysdeps_alloca(strlen(path) + 1);
    strcpy(dir, path);
    dirname(dir);
    if (dir[0] != '\0') {
        make_dirs(env, dir);
    }
    YogSysdeps_mkdir(path);
}

static YogVal
decompress(YogEnv* env, YogVal self, YogVal pkg, YogVal args, YogVal kw, YogVal block)
{
    SAVE_ARGS5(env, self, pkg, args, kw, block);
    YogVal zip = YUNDEF;
    YogVal dest = YNIL;
    YogVal s = YUNDEF;
    PUSH_LOCALS3(env, zip, dest, s);
    YogCArg params[] = {
        { "zip", &zip },
        { "|", NULL },
        { "dest", &dest },
        { NULL, NULL } };
    YogGetArgs_parse_args(env, "decompress", params, args, kw);
    if (!IS_PTR(zip) || (BASIC_OBJ_TYPE(zip) != TYPE_STRING)) {
        YogError_raise_TypeError(env, "zip must be String");
    }
    if (!IS_NIL(dest) && (!IS_PTR(dest) || (BASIC_OBJ_TYPE(dest) != TYPE_STRING))) {
        YogError_raise_TypeError(env, "dest must be nil or String");
    }

    struct zip* archive = open_zip(env, pkg, zip, 0);
    int n = zip_get_num_files(archive);
    YogHandle* h = VAL2HDL(env, dest);
    int i;
    for (i = 0; i < n; i++) {
        const char* name = zip_get_name(archive, i, 0);
        if (name == NULL) {
            close_zip(env, pkg, archive);
            raise_ZipError(env, pkg, zip_strerror(archive));
        }
        YogVal bin = YogString_to_bin_in_default_encoding(env, h);
        char* path = (char*)YogSysdeps_alloca(strlen(BINARY_CSTR(bin)) + strlen(name) + 2);
        strcpy(path, BINARY_CSTR(bin));
        strcat(path, "/");
        strcat(path, name);
        char* dir = (char*)YogSysdeps_alloca(strlen(path) + 1);
        strcpy(dir, path);
        dirname(dir);
        make_dirs(env, dir);

        struct zip_file* file = zip_fopen_index(archive, i, 0);
        if (file == NULL) {
            close_zip(env, pkg, archive);
            raise_ZipError(env, pkg, zip_strerror(archive));
        }
        FILE* fp = fopen(path, "wb");
        if (fp == NULL) {
            zip_fclose(file);
            close_zip(env, pkg, archive);
            s = YogString_from_string(env, path);
            YogError_raise_sys_err(env, errno, s);
        }
        while (1) {
            char buf[1024];
            int len = zip_fread(file, buf, array_sizeof(buf));
            if (len < 0) {
                zip_fclose(file);
                close_zip(env, pkg, archive);
                raise_ZipError(env, pkg, zip_strerror(archive));
            }
            else if (len == 0) {
                break;
            }
            fwrite(buf, sizeof(char), len, fp);
            if (ferror(fp)) {
                zip_fclose(file);
                close_zip(env, pkg, archive);
                YogError_raise_IOError(env, name);
            }
        }

        fclose(fp);
        if (zip_fclose(file) != 0) {
            close_zip(env, pkg, archive);
            raise_ZipError(env, pkg, zip_strerror(archive));
        }
    }
    close_zip(env, pkg, archive);

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
    YogGC_UPDATE_PTR(env, PTR_AS(Package, pkg), eZipError, eZipError);

    YogPackage_define_function(env, pkg, "compress", compress);
    YogPackage_define_function(env, pkg, "decompress", decompress);

    RETURN(env, pkg);
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
