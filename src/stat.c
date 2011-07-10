#include "yog/config.h"
#include <strings.h>
#include <sys/stat.h>
#include <sys/types.h>
#include "yog/binary.h"
#include "yog/class.h"
#include "yog/error.h"
#include "yog/gc.h"
#include "yog/handle.h"
#include "yog/object.h"
#include "yog/string.h"
#include "yog/vm.h"
#include "yog/yog.h"

struct Stat {
    struct YogBasicObj base;
    struct stat st;
};

typedef struct Stat Stat;

#define TYPE_STAT TO_TYPE(alloc)

static YogVal
alloc(YogEnv* env, YogVal klass)
{
    SAVE_ARG(env, klass);
    YogVal st = ALLOC_OBJ(env, YogBasicObj_keep_children, NULL, Stat);
    PUSH_LOCAL(env, st);
    YogBasicObj_init(env, st, TYPE_STAT, 0, klass);
    bzero(&PTR_AS(Stat, st)->st, sizeof(PTR_AS(Stat, st)->st));
    RETURN(env, st);
}

static YogVal
do_stat(YogEnv* env, YogHandle* path, int (*f)(const char*, struct stat*))
{
    YogHandle* st = VAL2HDL(env, alloc(env, env->vm->cStat));
    YogVal s = YogString_to_bin_in_default_encoding(env, path);
    if (f(BINARY_CSTR(s), &HDL_AS(Stat, st)->st) != 0) {
        YogError_raise_sys_err(env, errno, HDL2VAL(path));
    }
    return HDL2VAL(st);
}

YogVal
YogStat_stat(YogEnv* env, YogHandle* path)
{
    return do_stat(env, path, stat);
}

YogVal
YogStat_lstat(YogEnv* env, YogHandle* path)
{
    return do_stat(env, path, lstat);
}

static YogVal
get_dir(YogEnv* env, YogHandle* self, YogHandle* pkg)
{
    return S_ISDIR(HDL_AS(Stat, self)->st.st_mode) ? YTRUE : YFALSE;
}

void
YogStat_define_classes(YogEnv* env, YogHandle* pkg)
{
    YogVM* vm = env->vm;
    YogHandle* cStat = VAL2HDL(env, YogClass_new(env, "Stat", vm->cObject));
#define DEFINE_PROP(name, getter, setter) do { \
    YogClass_define_property2(env, cStat, pkg, (name), (getter), (setter)); \
} while (0)
    DEFINE_PROP("dir?", get_dir, NULL);
#undef DEFINE_PROP
    vm->cStat = HDL2VAL(cStat);
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
