#include <errno.h>
#include <setjmp.h>
#include <stdio.h>
#include <string.h>
#include "yog/array.h"
#include "yog/class.h"
#include "yog/error.h"
#include "yog/file.h"
#include "yog/function.h"
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

static YogVal
allocate(YogEnv* env, YogVal klass)
{
    SAVE_ARG(env, klass);
    YogVal file = YUNDEF;
    PUSH_LOCAL(env, file);

    file = ALLOC_OBJ(env, YogBasicObj_keep_children, NULL, YogFile);
    YogBasicObj_init(env, file, TYPE_FILE, 0, klass);
    PTR_AS(YogFile, file)->fp = NULL;

    RETURN(env, file);
}

static YogVal
YogFile_new(YogEnv* env)
{
    SAVE_LOCALS(env);
    YogVal file = YUNDEF;
    PUSH_LOCAL(env, file);

    file = allocate(env, env->vm->cFile);

    RETURN(env, file);
}

static YogVal
write(YogEnv* env, YogVal self, YogVal pkg, YogVal args, YogVal kw, YogVal block)
{
    SAVE_ARGS5(env, self, pkg, args, kw, block);
    YogVal s = YUNDEF;
    PUSH_LOCAL(env, s);

    YogCArg params[] = { { "s", &s }, { NULL, NULL } };
    YogGetArgs_parse_args(env, "write", params, args, kw);
    CHECK_SELF_TYPE(env, self);
    if (!IS_PTR(s) || (BASIC_OBJ_TYPE(s) != TYPE_STRING)) {
        YogError_raise_TypeError(env, "first argument must be String");
    }

    fputs(STRING_CSTR(s), PTR_AS(YogFile, self)->fp);

    RETURN(env, self);
}

static YogVal
read(YogEnv* env, YogVal self, YogVal pkg, YogVal args, YogVal kw, YogVal block)
{
    SAVE_ARGS5(env, self, pkg, args, kw, block);
    YogVal s = YUNDEF;
    PUSH_LOCAL(env, s);

    YogCArg params[] = { { NULL, NULL } };
    YogGetArgs_parse_args(env, "read", params, args, kw);
    CHECK_SELF_TYPE(env, self);

    s = YogString_new(env);

    FILE* fp = PTR_AS(YogFile, self)->fp;
    do {
        char buffer[4096];
        uint_t size = fread(buffer, sizeof(char), array_sizeof(buffer) - 1, fp);
        buffer[size] = '\0';
        YogString_add_cstr(env, s, buffer);
    } while (!feof(fp));

    RETURN(env, s);
}

static void
do_close(YogEnv* env, YogVal self)
{
    SAVE_ARG(env, self);
    fclose(PTR_AS(YogFile, self)->fp);
    PTR_AS(YogFile, self)->fp = NULL;
    RETURN_VOID(env);
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

    YogCArg params[] = { { NULL, NULL } };
    YogGetArgs_parse_args(env, "readline", params, args, kw);
    CHECK_SELF_TYPE(env, self);

    FILE* fp = PTR_AS(YogFile, self)->fp;
    char buffer[4096];
#define FGETS   fgets(buffer, array_sizeof(buffer), fp)
    if (FGETS == NULL) {
        YogError_raise_EOFError(env, "end of file reached");
    }
    line = YogString_new_str(env, buffer);

    while (buffer[strlen(buffer) - 1] != '\n') {
        if (FGETS == NULL) {
            break;
        }
        YogString_add_cstr(env, line, buffer);
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
    YogVal retval = YUNDEF;
    PUSH_LOCALS4(env, file, path, mode, retval);

    YogCArg params[] = { { "path", &path }, { "mode", &mode }, { NULL, NULL } };
    YogGetArgs_parse_args(env, "open", params, args, kw);
    if (!IS_PTR(self) || (BASIC_OBJ_TYPE(self) != TYPE_CLASS)) {
        YogError_raise_TypeError(env, "self must be Class");
    }
    if (!IS_PTR(path) || (BASIC_OBJ_TYPE(path) != TYPE_STRING)) {
        YogError_raise_TypeError(env, "path must be String");
    }
    if (!IS_PTR(mode) || (BASIC_OBJ_TYPE(mode) != TYPE_STRING)) {
        YogError_raise_TypeError(env, "mode must be String");
    }

    file = YogFile_new(env);

    FILE* fp = fopen(STRING_CSTR(path), STRING_CSTR(mode));
    if (fp == NULL) {
        YogError_raise_sys_call_err(env, errno);
    }
    PTR_AS(YogFile, file)->fp = fp;

    if (!IS_PTR(block)) {
        RETURN(env, file);
    }

    SAVE_CURRENT_STAT(env, open);

    YogJmpBuf jmpbuf;
    int_t status;
    if ((status = setjmp(jmpbuf.buf)) == 0) {
        PUSH_JMPBUF(env->thread, jmpbuf);
        retval = YogCallable_call1(env, block, file);
        do_close(env, file);
        POP_JMPBUF(env);
    }
    else {
        RESTORE_STAT(env, open);
        do_close(env, file);
        LONGJMP(env, status);
    }

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
    YogClass_define_allocator(env, cFile, allocate);
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
    DEFINE_METHOD("write", write);
#undef DEFINE_METHOD
    vm->cFile = cFile;

    RETURN_VOID(env);
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
