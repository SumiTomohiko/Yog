#include <errno.h>
#include <setjmp.h>
#include <stdio.h>
#include <string.h>
#include <sys/file.h>
#include "yog/array.h"
#include "yog/binary.h"
#include "yog/callable.h"
#include "yog/class.h"
#include "yog/encoding.h"
#include "yog/error.h"
#include "yog/eval.h"
#include "yog/file.h"
#include "yog/get_args.h"
#include "yog/misc.h"
#include "yog/object.h"
#include "yog/string.h"
#include "yog/string.h"
#include "yog/thread.h"
#include "yog/vm.h"
#include "yog/yog.h"

#define CHECK_SELF_TYPE(env, self)  do { \
    if (!IS_PTR(self) || (BASIC_OBJ_TYPE(self) != TYPE_FILE)) { \
        YogError_raise_TypeError((env), "self must be File"); \
    } \
} while (0)
#define CHECK_SELF_TYPE2(env, self)  do { \
    YogVal obj = HDL2VAL(self); \
    if (!IS_PTR(obj) || (BASIC_OBJ_TYPE(obj) != TYPE_FILE)) { \
        YogError_raise_TypeError((env), "self must be File"); \
    } \
} while (0)

static void
YogFile_keep_children(YogEnv* env, void* ptr, ObjectKeeper keeper, void* heap)
{
    YogBasicObj_keep_children(env, ptr, keeper, heap);

    YogFile* file = (YogFile*)ptr;
#define KEEP(member)    YogGC_KEEP(env, file, member, keeper, heap)
    KEEP(encoding);
#undef KEEP
}

static YogVal
alloc(YogEnv* env, YogVal klass)
{
    SAVE_ARG(env, klass);
    YogVal file = YUNDEF;
    PUSH_LOCAL(env, file);

    file = ALLOC_OBJ(env, YogFile_keep_children, NULL, YogFile);
    YogBasicObj_init(env, file, TYPE_FILE, 0, klass);
    PTR_AS(YogFile, file)->fp = NULL;
    PTR_AS(YogFile, file)->encoding = YUNDEF;

    RETURN(env, file);
}

YogVal
YogFile_new(YogEnv* env, FILE* fp)
{
    YogVal file = alloc(env, env->vm->cFile);
    PTR_AS(YogFile, file)->fp = fp;
    return file;
}

static void
write_binary(YogEnv* env, YogHandle* self, YogVal val)
{
    FILE* fp = HDL_AS(YogFile, self)->fp;
    uint_t rest = BINARY_SIZE(val);
    const char* p = BINARY_CSTR(val);
    while (0 < rest) {
        uint_t size = fwrite(p, 1, rest, fp);
        rest -= size;
        p += size;
    }
}

static uint_t
read_binary_to_append(YogEnv* env, YogHandle* bin, FILE* fp, size_t size)
{
    char buffer[size];
    size_t nbytes = fread(buffer, sizeof(buffer[0]), size, fp);
    if (ferror(fp)) {
        YogError_raise_sys_err(env, errno, YUNDEF);
    }
    YogBinary_add(env, HDL2VAL(bin), buffer, nbytes);
    return nbytes;
}

static YogVal
read_binary_all(YogEnv* env, FILE* fp)
{
    YogHandle* data = VAL2HDL(env, YogBinary_new(env));
    while (!feof(fp)) {
        read_binary_to_append(env, data, fp, 4096);
    }
    return HDL2VAL(data);
}

static YogVal
read_binary(YogEnv* env, YogHandle* self, YogHandle* pkg, YogHandle* size)
{
    CHECK_SELF_TYPE2(env, self);
    if (size == NULL) {
        return read_binary_all(env, HDL_AS(YogFile, self)->fp);
    }
    YogMisc_check_Fixnum(env, size, "size");
    if (VAL2INT(HDL2VAL(size)) < 0) {
        const char* fmt = "size must be equal to or greater than zero, not %d";
        YogError_raise_ValueError(env, fmt, VAL2INT(HDL2VAL(size)));
    }

    uint_t rest = (uint_t)VAL2INT(HDL2VAL(size));
    YogHandle* bin = VAL2HDL(env, YogBinary_new(env));
    FILE* fp = HDL_AS(YogFile, self)->fp;
    while (!feof(fp) && (0 < rest)) {
        uint_t max = 4096;
        uint_t size = max < rest ? max : rest;
        uint_t nbytes = read_binary_to_append(env, bin, fp, size);
        rest -= nbytes;
    }
    return HDL2VAL(bin);
}

static void
do_unlock(YogEnv* env, YogHandle* self)
{
    if (flock(fileno(HDL_AS(YogFile, self)->fp), LOCK_UN) != 0) {
        YogError_raise_sys_err(env, errno, YUNDEF);
    }
}

static YogVal
do_lock(YogEnv* env, YogHandle* self, YogHandle* block, int operation)
{
    CHECK_SELF_TYPE2(env, self);
    if (flock(fileno(HDL_AS(YogFile, self)->fp), operation) != 0) {
        YogError_raise_sys_err(env, errno, YUNDEF);
    }

    YogJmpBuf jmpbuf;
    int_t status = setjmp(jmpbuf.buf);
    if (status == 0) {
        INIT_JMPBUF(env, jmpbuf);
        PUSH_JMPBUF(env->thread, jmpbuf);

        YogVal retval = YogCallable_call1(env, HDL2VAL(block), HDL2VAL(self));
        do_unlock(env, self);

        POP_JMPBUF(env);
        return retval;
    }

    do_unlock(env, self);
    YogEval_longjmp_to_prev_buf(env, status);
    /* NOTREACHED */
    return YUNDEF;
}

static YogVal
lock_exclusive(YogEnv* env, YogHandle* self, YogHandle* pkg, YogHandle* block)
{
    return do_lock(env, self, block, LOCK_EX);
}

static YogVal
lock_shared(YogEnv* env, YogHandle* self, YogHandle* pkg, YogHandle* block)
{
    return do_lock(env, self, block, LOCK_SH);
}

static YogVal
flush(YogEnv* env, YogHandle* self, YogHandle* pkg)
{
    CHECK_SELF_TYPE2(env, self);
    if (fflush(HDL_AS(YogFile, self)->fp) != 0) {
        YogError_raise_sys_err(env, errno, YUNDEF);
    }
    return HDL2VAL(self);
}

static YogVal
write_(YogEnv* env, YogHandle* self, YogHandle* pkg, YogHandle* data)
{
    CHECK_SELF_TYPE2(env, self);
    YOG_ASSERT(env, data != NULL, "data is not optional.");
    YogVal val = HDL2VAL(data);
#define RAISE_TYPE_ERROR    do { \
    const char* msg = "data must be Binary or String, not %C"; \
    YogError_raise_TypeError(env, msg, data); \
} while (0)
    if (!IS_PTR(val)) {
        RAISE_TYPE_ERROR;
    }
    if (BASIC_OBJ_TYPE(val) == TYPE_BINARY) {
        write_binary(env, self, val);
        return HDL2VAL(self);
    }
    if (BASIC_OBJ_TYPE(val) != TYPE_STRING) {
        RAISE_TYPE_ERROR;
    }
#undef RAISE_TYPE_ERROR

    YogVal bin = YogString_to_bin_in_default_encoding(env, data);
    fputs(BINARY_CSTR(bin), HDL_AS(YogFile, self)->fp);

    return HDL2VAL(self);
}

static uint_t
read_to_append(YogEnv* env, YogVal s, FILE* fp, size_t size)
{
    char buffer[size + 1];
    uint_t nbytes = fread(buffer, sizeof(buffer[0]), size, fp);
    if (ferror(fp)) {
        YogError_raise_sys_err(env, errno, YNIL);
    }
    buffer[nbytes] = '\0';
    YogString_append_string(env, s, buffer);
    return nbytes;
}

static YogVal
read_all(YogEnv* env, YogVal self)
{
    SAVE_ARG(env, self);
    YogVal s = YogString_new(env);
    PUSH_LOCAL(env, s);

    FILE* fp = PTR_AS(YogFile, self)->fp;
    do {
        read_to_append(env, s, fp, 4096);
    } while (!feof(fp));

    RETURN(env, s);
}

static YogVal
read_(YogEnv* env, YogVal self, YogVal pkg, YogVal args, YogVal kw, YogVal block)
{
    SAVE_ARGS5(env, self, pkg, args, kw, block);
    YogVal s = YUNDEF;
    YogVal size = YNIL;
    PUSH_LOCALS2(env, s, size);
    CHECK_SELF_TYPE(env, self);
    YogCArg params[] = { { "|", NULL }, { "size", &size }, { NULL, NULL } };
    YogGetArgs_parse_args(env, "read", params, args, kw);
    if (!IS_NIL(size) && !IS_FIXNUM(size)) {
        const char* fmt = "size must be Fixnum or Nil, not %C";
        YogError_raise_TypeError(env, fmt, size);
    }
    if (IS_FIXNUM(size) && (VAL2INT(size) < 0)) {
        const char* fmt = "size must be equal to or greater than zero, not %d";
        YogError_raise_ValueError(env, fmt, VAL2INT(size));
    }

    if (IS_UNDEF(PTR_AS(YogFile, self)->encoding)) {
        RETURN(env, read_binary_all(env, PTR_AS(YogFile, self)->fp));
    }

    if (IS_NIL(size)) {
        RETURN(env, read_all(env, self));
    }

    s = YogString_new(env);
    FILE* fp = PTR_AS(YogFile, self)->fp;
    int_t rest = VAL2INT(size);
    do {
        size_t max = 4096;
        uint_t nbytes = read_to_append(env, s, fp, max < rest ? max : rest);
        rest -= nbytes;
    } while (!feof(fp) && (0 < rest));

    RETURN(env, s);
}

static void
do_close(YogEnv* env, YogVal self)
{
    if (fclose(PTR_AS(YogFile, self)->fp) == 0) {
        PTR_AS(YogFile, self)->fp = NULL;
        return;
    }
    YogError_raise_sys_err(env, errno, YUNDEF);
}

static YogVal
close_(YogEnv* env, YogVal self, YogVal pkg, YogVal args, YogVal kw, YogVal block)
{
    SAVE_ARGS5(env, self, pkg, args, kw, block);

    YogCArg params[] = { { NULL, NULL } };
    YogGetArgs_parse_args(env, "close", params, args, kw);
    CHECK_SELF_TYPE(env, self);

    do_close(env, self);

    RETURN(env, self);
}

static YogVal
readline(YogEnv* env, YogVal self, YogVal pkg, YogVal args, YogVal kw, YogVal block)
{
    SAVE_ARGS5(env, self, pkg, args, kw, block);
    YogVal line = YUNDEF;
    PUSH_LOCAL(env, line);
    CHECK_SELF_TYPE(env, self);
    YogCArg params[] = { { NULL, NULL } };
    YogGetArgs_parse_args(env, "readline", params, args, kw);

    FILE* fp = PTR_AS(YogFile, self)->fp;
    char buffer[4096];
#define FGETS   fgets(buffer, array_sizeof(buffer), fp)
    if (FGETS == NULL) {
        RETURN(env, YNIL);
    }
    line = YogString_new(env);
    YogString_append_string(env, line, buffer);

    while (buffer[strlen(buffer) - 1] != '\n') {
        if (FGETS == NULL) {
            break;
        }
        YogString_append_string(env, line, buffer);
    }
#undef FGETS

    RETURN(env, line);
}

static const char* r = "r";

static const char*
mode2cstr(YogEnv* env, YogVal mode)
{
    if (IS_UNDEF(mode)) {
        return r;
    }
    YogHandle* h = VAL2HDL(env, mode);
    return BINARY_CSTR(YogString_to_bin_in_default_encoding(env, h));
}

static YogVal
decide_encoding(YogEnv* env, const char* mode, YogVal encoding)
{
    if (strchr(mode, 'b') != NULL) {
        return YUNDEF;
    }

    if (IS_UNDEF(encoding) || IS_NIL(encoding)) {
        return YogEncoding_get_default(env);
    }
    return encoding;
}

static YogVal
open_(YogEnv* env, YogVal self, YogVal pkg, YogVal args, YogVal kw, YogVal block)
{
    SAVE_ARGS5(env, self, pkg, args, kw, block);
    YogVal file = YUNDEF;
    YogVal path = YUNDEF;
    YogVal mode = YUNDEF;
    YogVal encoding = YUNDEF;
    YogVal retval = YUNDEF;
    YogVal fp_enc = YUNDEF;
    PUSH_LOCALS6(env, file, path, mode, encoding, retval, fp_enc);

    YogCArg params[] = {
        { "path", &path },
        { "|", NULL },
        { "mode", &mode },
        { "encoding", &encoding },
        { NULL, NULL } };
    YogGetArgs_parse_args(env, "open", params, args, kw);
    if (!IS_PTR(self) || ((BASIC_OBJ_FLAGS(self) & FLAG_CLASS) == 0)) {
        YogError_raise_TypeError(env, "self must be Class");
    }
    if (!IS_PTR(path) || (BASIC_OBJ_TYPE(path) != TYPE_STRING)) {
        YogError_raise_TypeError(env, "path must be String");
    }
    if (!IS_UNDEF(mode) && (!IS_PTR(mode) || (BASIC_OBJ_TYPE(mode) != TYPE_STRING))) {
        YogError_raise_TypeError(env, "mode must be String");
    }
    char m[3];
    strncpy(m, mode2cstr(env, mode), array_sizeof(m));
#if 0
    else if (!IS_PTR(encoding) || (BASIC_OBJ_TYPE(encoding) != TYPE_ENCODING)) {
        YogError_raise_TypeError(env, "encoding must be Encoding");
    }
#endif


    YogHandle* h = YogHandle_REGISTER(env, path);
    YogHandle* bin = VAL2HDL(env, YogString_to_bin_in_default_encoding(env, h));
    FILE* fp = fopen(BINARY_CSTR(HDL2VAL(bin)), m);
    if (fp == NULL) {
        YogError_raise_sys_err(env, errno, path);
    }
    file = YogFile_new(env, fp);
    fp_enc = decide_encoding(env, m, encoding);
    YogGC_UPDATE_PTR(env, PTR_AS(YogFile, file), encoding, fp_enc);

    if (!IS_PTR(block)) {
        RETURN(env, file);
    }

    YogJmpBuf jmpbuf;
    int_t status = setjmp(jmpbuf.buf);
    if (status == 0) {
        INIT_JMPBUF(env, jmpbuf);
        PUSH_JMPBUF(env->thread, jmpbuf);

        retval = YogCallable_call1(env, block, file);
        do_close(env, file);

        POP_JMPBUF(env);
        RETURN(env, retval);
    }

    do_close(env, file);
    YogEval_longjmp_to_prev_buf(env, status);
    RETURN(env, retval);
}

static YogVal
get_eof(YogEnv* env, YogVal self, YogVal pkg, YogVal args, YogVal kw, YogVal block)
{
    SAVE_ARGS5(env, self, pkg, args, kw, block);
    YogCArg params[] = { { NULL, NULL } };
    YogGetArgs_parse_args(env, "get_eof", params, args, kw);
    CHECK_SELF_TYPE(env, self);
    RETURN(env, feof(PTR_AS(YogFile, self)->fp) ? YTRUE : YFALSE);
}

void
YogFile_define_classes(YogEnv* env, YogVal pkg)
{
    SAVE_ARG(env, pkg);
    YogVal cFile = YUNDEF;
    PUSH_LOCAL(env, cFile);
    YogVM* vm = env->vm;

    cFile = YogClass_new(env, "File", vm->cObject);
    YogClass_define_allocator(env, cFile, alloc);
#define DEFINE_CLASS_METHOD(name, f)    do { \
    YogClass_define_class_method(env, cFile, pkg, (name), (f)); \
} while (0)
    DEFINE_CLASS_METHOD("open", open_);
#undef DEFINE_CLASS_METHOD
#define DEFINE_METHOD(name, f)  do { \
    YogClass_define_method(env, cFile, pkg, (name), (f)); \
} while (0)
    DEFINE_METHOD("close", close_);
    DEFINE_METHOD("read", read_);
    DEFINE_METHOD("readline", readline);
#undef DEFINE_METHOD
#define DEFINE_METHOD2(name, f, ...) do { \
    YogClass_define_method2(env, cFile, pkg, (name), (f), __VA_ARGS__); \
} while (0)
    DEFINE_METHOD2("flush", flush, NULL);
    DEFINE_METHOD2("lock_exclusive", lock_exclusive, "&", NULL);
    DEFINE_METHOD2("lock_shared", lock_shared, "&", NULL);
    DEFINE_METHOD2("read_binary", read_binary, "|", "size", NULL);
    DEFINE_METHOD2("write", write_, "data", NULL);
#undef DEFINE_METHOD2
#define DEFINE_PROP(name, getter, setter) do { \
    YogClass_define_property(env, cFile, pkg, (name), (getter), (setter)); \
} while (0)
    DEFINE_PROP("eof?", get_eof, NULL);
#undef DEFINE_PROP
    vm->cFile = cFile;

    RETURN_VOID(env);
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
