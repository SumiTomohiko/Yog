#include "yog/config.h"
#include <dirent.h>
#include <sys/types.h>
#include "yog/binary.h"
#include "yog/callable.h"
#include "yog/class.h"
#include "yog/error.h"
#include "yog/eval.h"
#include "yog/gc.h"
#include "yog/handle.h"
#include "yog/misc.h"
#include "yog/object.h"
#include "yog/string.h"
#include "yog/thread.h"
#include "yog/vm.h"

struct Dir {
    struct YogBasicObj base;
    DIR* dir;
};

typedef struct Dir Dir;

#define TYPE_DIR TO_TYPE(alloc)

static void
Dir_finalize(YogEnv* env, void* ptr)
{
    Dir* dir = (Dir*)ptr;
    if (dir->dir == NULL) {
        return;
    }
    closedir(dir->dir);
    dir->dir = NULL;
}

static YogVal
alloc(YogEnv* env, YogVal klass)
{
    SAVE_ARG(env, klass);
    YogVal dir = ALLOC_OBJ(env, YogBasicObj_keep_children, Dir_finalize, Dir);
    PUSH_LOCAL(env, dir);
    YogBasicObj_init(env, dir, TYPE_DIR, 0, klass);
    PTR_AS(Dir, dir)->dir = NULL;
    RETURN(env, dir);
}

static void
do_close(YogEnv* env, YogHandle* self)
{
    DIR* dir = HDL_AS(Dir, self)->dir;
    HDL_AS(Dir, self)->dir = NULL;
    if (closedir(dir) != 0) {
        YogError_raise_sys_err(env, errno, YNIL);
    }
}

static YogVal
open_(YogEnv* env, YogHandle* self, YogHandle* pkg, YogHandle* path, YogHandle* block)
{
    YogMisc_check_String(env, path, "path");
    YogHandle* dir = VAL2HDL(env, alloc(env, HDL2VAL(self)));
    YogVal s = YogString_to_bin_in_default_encoding(env, path);
    DIR* dirp = opendir(BINARY_CSTR(s));
    if (dirp == NULL) {
        YogError_raise_sys_err(env, errno, HDL2VAL(path));
    }
    HDL_AS(Dir, dir)->dir = dirp;

    YogJmpBuf jmpbuf;
    int_t status = setjmp(jmpbuf.buf);
    if (status == 0) {
        INIT_JMPBUF(env, jmpbuf);
        PUSH_JMPBUF(env->thread, jmpbuf);

        YogVal retval = YogCallable_call1(env, HDL2VAL(block), HDL2VAL(dir));
        YogHandle* h = VAL2HDL(env, retval);

        do_close(env, dir);
        POP_JMPBUF(env);
        return HDL2VAL(h);
    }

    do_close(env, dir);
    YogEval_longjmp_to_prev_buf(env, status);
    /* NOTREACHED */
    return YUNDEF;
}

static YogVal
each(YogEnv* env, YogHandle* self, YogHandle* pkg, YogHandle* block)
{
    struct dirent* de;
    while ((de = readdir(HDL_AS(Dir, self)->dir)) != NULL) {
        YogVal name = YogString_from_string(env, de->d_name);
        YogCallable_call1(env, HDL2VAL(block), name);
    }
    return HDL2VAL(self);
}

void
YogDir_define_classes(YogEnv* env, YogHandle* pkg)
{
    YogVM* vm = env->vm;

    YogHandle* cDir = VAL2HDL(env, YogClass_new(env, "Dir", vm->cObject));
    YogClass_define_allocator(env, HDL2VAL(cDir), alloc);
#define DEFINE_METHOD(name, ...) do { \
    YogClass_define_method2(env, HDL2VAL(cDir), HDL2VAL(pkg), (name), __VA_ARGS__); \
} while (0)
    DEFINE_METHOD("each", each, "&", NULL);
#undef DEFINE_METHOD
#define DEFINE_CLASS_METHOD(name, ...) do { \
    YogClass_define_class_method2(env, cDir, pkg, (name), __VA_ARGS__); \
} while (0)
    DEFINE_CLASS_METHOD("open", open_, "path", "&", NULL);
#undef DEFINE_CLASS_METHOD
    vm->cDir = HDL2VAL(cDir);
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
