#include <setjmp.h>
#include <stdio.h>
#include <string.h>
#include "yog/array.h"
#include "yog/error.h"
#include "yog/file.h"
#include "yog/function.h"
#include "yog/klass.h"
#include "yog/object.h"
#include "yog/string.h"
#include "yog/string.h"
#include "yog/thread.h"
#include "yog/yog.h"

static YogVal
allocate(YogEnv* env, YogVal klass)
{
    SAVE_ARG(env, klass);
    YogVal file = YUNDEF;
    PUSH_LOCAL(env, file);

    file = ALLOC_OBJ(env, YogBasicObj_keep_children, NULL, YogFile);
    YogBasicObj_init(env, file, 0, klass);
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
read(YogEnv* env, YogVal self, YogVal args, YogVal kw, YogVal block)
{
    SAVE_ARGS4(env, self, args, kw, block);
    YogVal s = YUNDEF;
    PUSH_LOCAL(env, s);

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
    RETURN_VOID(env);
}

static YogVal
close(YogEnv* env, YogVal self, YogVal args, YogVal kw, YogVal block)
{
    SAVE_ARGS4(env, self, args, kw, block);
    do_close(env, self);
    RETURN(env, self);
}

static YogVal
readline(YogEnv* env, YogVal self, YogVal args, YogVal kw, YogVal block)
{
    SAVE_ARGS4(env, self, args, kw, block);
    YogVal line = YUNDEF;
    PUSH_LOCAL(env, line);

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
open(YogEnv* env, YogVal self, YogVal args, YogVal kw, YogVal block)
{
    SAVE_ARGS4(env, self, args, kw, block);
    YogVal file = YUNDEF;
    YogVal path = YUNDEF;
    YogVal mode = YUNDEF;
    YogVal retval = YUNDEF;
    PUSH_LOCALS4(env, file, path, mode, retval);

    path = YogArray_at(env, args, 0);
    YOG_ASSERT(env, IS_PTR(path), "invalid path");
    YOG_ASSERT(env, IS_OBJ_OF(env, path, cString), "invalid path type");
    mode = YogArray_at(env, args, 1);
    YOG_ASSERT(env, IS_PTR(mode), "invalid mode");
    YOG_ASSERT(env, IS_OBJ_OF(env, mode, cString), "invalid mode type");

    file = YogFile_new(env);

    FILE* fp = fopen(STRING_CSTR(path), STRING_CSTR(mode));
    YOG_ASSERT(env, fp != NULL, "can't open file");
    PTR_AS(YogFile, file)->fp = fp;

    if (!IS_PTR(block)) {
        RETURN(env, file);
    }

    YogJmpBuf jmpbuf;
    PUSH_JMPBUF(env->thread, jmpbuf);
    SAVE_CURRENT_STAT(env, open);

    int_t status;
    if ((status = setjmp(jmpbuf.buf)) == 0) {
        retval = YogCallable_call1(env, block, file);
        do_close(env, file);
    }
    else {
        RESTORE_STAT(env, open);

        do_close(env, file);

        YogJmpBuf* list = PTR_AS(YogThread, env->thread)->jmp_buf_list;
        YOG_ASSERT(env, list != NULL, "no longjmp destination");
        longjmp(list->buf, status);
    }

    RETURN(env, retval);
}

YogVal
YogFile_klass_new(YogEnv* env)
{
    SAVE_LOCALS(env);
    YogVal klass = YUNDEF;
    PUSH_LOCAL(env, klass);

    klass = YogKlass_new(env, "File", env->vm->cObject);
    YogKlass_define_allocator(env, klass, allocate);
    YogKlass_define_class_method(env, klass, "open", open);
    YogKlass_define_method(env, klass, "close", close);
    YogKlass_define_method(env, klass, "read", read);
    YogKlass_define_method(env, klass, "readline", readline);

    RETURN(env, klass);
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
