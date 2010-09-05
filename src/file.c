#include <errno.h>
#include <setjmp.h>
#include <stdio.h>
#include <string.h>
#include "yog/array.h"
#include "yog/binary.h"
#include "yog/callable.h"
#include "yog/class.h"
#include "yog/encoding.h"
#include "yog/error.h"
#include "yog/eval.h"
#include "yog/file.h"
#include "yog/get_args.h"
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
YogFile_new(YogEnv* env)
{
    return alloc(env, env->vm->cFile);
}

static YogVal
write(YogEnv* env, YogHandle* self, YogHandle* pkg, YogHandle* s)
{
    CHECK_SELF_TYPE2(env, self);
    if (!IS_PTR(HDL2VAL(s)) || (BASIC_OBJ_TYPE(HDL2VAL(s)) != TYPE_STRING)) {
        YogError_raise_TypeError(env, "First argument must be String");
    }

    YogVal bin = YogString_to_bin_in_default_encoding(env, s);
    fputs(BINARY_CSTR(bin), PTR_AS(YogFile, self)->fp);

    return HDL2VAL(self);
}

static YogVal
read(YogEnv* env, YogVal self, YogVal pkg, YogVal args, YogVal kw, YogVal block)
{
    SAVE_ARGS5(env, self, pkg, args, kw, block);
    YogVal s = YUNDEF;
    PUSH_LOCAL(env, s);
    CHECK_SELF_TYPE(env, self);
    YogCArg params[] = { { NULL, NULL } };
    YogGetArgs_parse_args(env, "read", params, args, kw);

    s = YogString_new(env);
    FILE* fp = PTR_AS(YogFile, self)->fp;
    do {
        char buffer[4096];
        uint_t size = fread(buffer, sizeof(char), array_sizeof(buffer) - 1, fp);
        buffer[size] = '\0';
        YogString_append_string(env, s, buffer);
    } while (!feof(fp));

    RETURN(env, s);
}

static void
do_close(YogEnv* env, YogVal self)
{
    fclose(PTR_AS(YogFile, self)->fp);
    PTR_AS(YogFile, self)->fp = NULL;
}

static YogVal
close(YogEnv* env, YogVal self, YogVal pkg, YogVal args, YogVal kw, YogVal block)
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

static YogVal
open(YogEnv* env, YogVal self, YogVal pkg, YogVal args, YogVal kw, YogVal block)
{
    SAVE_ARGS5(env, self, pkg, args, kw, block);
    YogVal file = YUNDEF;
    YogVal path = YUNDEF;
    YogVal mode = YUNDEF;
    YogVal encoding = YUNDEF;
    YogVal retval = YUNDEF;
    PUSH_LOCALS5(env, file, path, mode, encoding, retval);

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
    if (IS_UNDEF(encoding)) {
        /* TODO: IS_NIL */
        encoding = YogEncoding_get_default(env);
    }
#if 0
    else if (!IS_PTR(encoding) || (BASIC_OBJ_TYPE(encoding) != TYPE_ENCODING)) {
        YogError_raise_TypeError(env, "encoding must be Encoding");
    }
#endif

    file = YogFile_new(env);

    YogHandle* h = YogHandle_REGISTER(env, path);
    YogVal bin = YogString_to_bin_in_default_encoding(env, h);
    const char* m = IS_UNDEF(mode) ? "r" : BINARY_CSTR(YogString_to_bin_in_default_encoding(env, YogHandle_REGISTER(env, mode)));
    FILE* fp = fopen(BINARY_CSTR(bin), m);
    if (fp == NULL) {
        YogError_raise_sys_err(env, errno, path);
    }
    PTR_AS(YogFile, file)->fp = fp;
    YogGC_UPDATE_PTR(env, PTR_AS(YogFile, file), encoding, encoding);

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
    DEFINE_CLASS_METHOD("open", open);
#undef DEFINE_CLASS_METHOD
#define DEFINE_METHOD(name, f)  do { \
    YogClass_define_method(env, cFile, pkg, (name), (f)); \
} while (0)
    DEFINE_METHOD("close", close);
    DEFINE_METHOD("read", read);
    DEFINE_METHOD("readline", readline);
#undef DEFINE_METHOD
#define DEFINE_METHOD2(name, f, ...) do { \
    YogClass_define_method2(env, cFile, pkg, (name), (f), __VA_ARGS__); \
} while (0)
    DEFINE_METHOD2("write", write, "s", NULL);
#undef DEFINE_METHOD2
    vm->cFile = cFile;

    RETURN_VOID(env);
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
