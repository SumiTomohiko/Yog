#include "yog/config.h"
#include "yog/class.h"
#include "yog/gc.h"
#include "yog/object.h"
#include "yog/sysdeps.h"
#include "yog/vm.h"
#include "yog/yog.h"

struct Lib {
    struct YogBasicObj base;
    LIB_HANDLE handle;
};

typedef struct Lib Lib;

#define TYPE_LIB TO_TYPE(Lib_alloc)

static YogVal
Lib_alloc(YogEnv* env, YogVal klass)
{
    SAVE_ARG(env, klass);
    YogVal obj = YUNDEF;
    PUSH_LOCAL(env, obj);

    obj = ALLOC_OBJ(env, YogBasicObj_keep_children, NULL, Lib);
    YogBasicObj_init(env, obj, TYPE_LIB, 0, klass);
    PTR_AS(Lib, obj)->handle = NULL;

    RETURN(env, obj);
}

YogVal
YogFFI_open_lib(YogEnv* env, const char* path)
{
    SAVE_LOCALS(env);
    YogVal lib = YUNDEF;
    PUSH_LOCAL(env, lib);

    LIB_HANDLE handle = YogSysdeps_open_lib(path);
    if (handle == NULL) {
        YogError_raise_ImportError(env, "no library named \"%s\"", path);
    }
    lib = Lib_alloc(env, env->vm->cLib);
    PTR_AS(Lib, lib)->handle = handle;

    RETURN(env, lib);
}

void
YogFFI_define_classes(YogEnv* env, YogVal pkg)
{
    SAVE_ARG(env, pkg);
    YogVal cLib = YUNDEF;
    PUSH_LOCAL(env, cLib);
    YogVM* vm = env->vm;

    cLib = YogClass_new(env, "Lib", vm->cObject);
    YogClass_define_allocator(env, cLib, Lib_alloc);
    vm->cLib = cLib;

    RETURN_VOID(env);
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
