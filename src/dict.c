#include "yog/array.h"
#include "yog/dict.h"
#include "yog/error.h"
#include "yog/klass.h"
#include "yog/object.h"
#include "yog/table.h"
#include "yog/thread.h"
#include "yog/yog.h"

static YogVal
subscript(YogEnv* env, YogVal self, YogVal args, YogVal kw, YogVal block)
{
    SAVE_ARGS4(env, self, args, kw, block);
    YogVal key = YUNDEF;
    YogVal value = YUNDEF;
    PUSH_LOCALS2(env, key, value);

    key = YogArray_at(env, args, 0);
    if (YogTable_lookup(env, PTR_AS(YogDict, self)->tbl, key, &value)) {
        RETURN(env, value);
    }

    YogError_raise_KeyError(env, "%s", "");

    /* NOTREACHED */
    RETURN(env, YUNDEF);
}

static YogVal
subscript_assign(YogEnv* env, YogVal self, YogVal args, YogVal kw, YogVal block)
{
    SAVE_ARGS4(env, self, args, kw, block);
    YogVal key = YUNDEF;
    YogVal value = YUNDEF;
    YogVal tbl = YUNDEF;
    PUSH_LOCALS3(env, key, value, tbl);

    tbl = PTR_AS(YogDict, self)->tbl;
    key = YogArray_at(env, args, 0);
    value = YogArray_at(env, args, 1);
    YogTable_insert(env, tbl, key, value);

    RETURN(env, self);
}

static void
init(YogEnv* env, YogVal self, YogVal klass)
{
    SAVE_ARGS2(env, self, klass);
    YogVal tbl = YUNDEF;
    PUSH_LOCAL(env, tbl);

    YogBasicObj_init(env, self, 0, klass);
    tbl = YogTable_new_val_table(env);
    PTR_AS(YogDict, self)->tbl = tbl;

    RETURN_VOID(env);
}

static void
keep_children(YogEnv* env, void* ptr, ObjectKeeper keeper, void* heap)
{
    YogBasicObj_keep_children(env, ptr, keeper, heap);

    YogDict* dict = ptr;
#define KEEP(member)    YogGC_keep(env, &dict->member, keeper, heap)
    KEEP(tbl);
#undef KEEP
}

static YogVal
allocate(YogEnv* env, YogVal klass)
{
    SAVE_ARG(env, klass);
    YogVal d = YUNDEF;
    PUSH_LOCAL(env, d);

    d = ALLOC_OBJ(env, keep_children, NULL, YogDict);
    init(env, d, klass);

    RETURN(env, d);
}

YogVal
YogDict_klass_new(YogEnv* env)
{
    SAVE_LOCALS(env);
    YogVal klass = YUNDEF;
    PUSH_LOCAL(env, klass);

    klass = YogKlass_new(env, "Dict", env->vm->cObject);
    YogKlass_define_allocator(env, klass, allocate);
#define DEFINE_METHOD(name, f)  YogKlass_define_method(env, klass, name, f)
    DEFINE_METHOD("[]", subscript);
    DEFINE_METHOD("[]=", subscript_assign);
#undef DEFINE_METHOD

    RETURN(env, klass);
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
