#include "yog/array.h"
#include "yog/dict.h"
#include "yog/error.h"
#include "yog/klass.h"
#include "yog/object.h"
#include "yog/table.h"
#include "yog/thread.h"
#include "yog/yog.h"

struct DictIterator {
    YogVal tbl_iter;
};

typedef struct DictIterator DictIterator;

static YogVal
add(YogEnv* env, YogVal self, YogVal args, YogVal kw, YogVal block)
{
    SAVE_ARGS4(env, self, args, kw, block);
    YogVal right = YUNDEF;
    YogVal key = YUNDEF;
    YogVal value = YUNDEF;
    YogVal iter = YUNDEF;
    YogVal dict = YUNDEF;
    PUSH_LOCALS5(env, right, key, value, iter, dict);

    right = YogArray_at(env, args, 0);
    YOG_ASSERT(env, IS_PTR(right), "invalid operand");
    YOG_ASSERT(env, IS_OBJ_OF(env, right, cDict), "invalid operand");

    dict = YogDict_new(env);

#define ADD(from)   do { \
    iter = YogDict_get_iterator(env, from); \
    while (YogDictIterator_next(env, iter)) { \
        key = YogDictIterator_current_key(env, iter); \
        value = YogDictIterator_current_value(env, iter); \
        YogDict_set(env, dict, key, value); \
    } \
} while (0)
    ADD(self);
    ADD(right);
#undef ADD

    RETURN(env, dict);
}

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

void
YogDict_set(YogEnv* env, YogVal self, YogVal key, YogVal value)
{
    SAVE_ARGS3(env, self, key, value);
    YogVal tbl = YUNDEF;
    PUSH_LOCAL(env, tbl);

    tbl = PTR_AS(YogDict, self)->tbl;
    YogTable_insert(env, tbl, key, value);

    RETURN_VOID(env);
}

static YogVal
subscript_assign(YogEnv* env, YogVal self, YogVal args, YogVal kw, YogVal block)
{
    SAVE_ARGS4(env, self, args, kw, block);
    YogVal key = YUNDEF;
    YogVal value = YUNDEF;
    PUSH_LOCALS2(env, key, value);

    key = YogArray_at(env, args, 0);
    value = YogArray_at(env, args, 1);
    YogDict_set(env, self, key, value);

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
YogDict_new(YogEnv* env)
{
    return allocate(env, env->vm->cDict);
}

static YogVal
get_size(YogEnv* env, YogVal self, YogVal args, YogVal kw, YogVal block)
{
    SAVE_ARGS4(env, self, args, kw, block);
    YogVal retval = YUNDEF;
    YogVal tbl = YUNDEF;
    PUSH_LOCALS2(env, retval, tbl);

    tbl = PTR_AS(YogDict, self)->tbl;
    int_t size = YogTable_size(env, tbl);
    retval = YogVal_from_int(env, size);

    RETURN(env, retval);
}

static void
DictIterator_keep_children(YogEnv* env, void* ptr, ObjectKeeper keeper, void* heap)
{
    DictIterator* iter = ptr;
#define KEEP(member)    YogGC_keep(env, &iter->member, keeper, heap)
    KEEP(tbl_iter);
#undef KEEP
}

YogVal
YogDictIterator_current_value(YogEnv* env, YogVal self)
{
    SAVE_ARG(env, self);
    YogVal tbl_iter = YUNDEF;
    YogVal retval = YUNDEF;
    PUSH_LOCALS2(env, tbl_iter, retval);

    tbl_iter = PTR_AS(DictIterator, self)->tbl_iter;
    retval = YogTableIterator_current_value(env, tbl_iter);

    RETURN(env, retval);
}

YogVal
YogDictIterator_current_key(YogEnv* env, YogVal self)
{
    SAVE_ARG(env, self);
    YogVal tbl_iter = YUNDEF;
    YogVal retval = YUNDEF;
    PUSH_LOCALS2(env, tbl_iter, retval);

    tbl_iter = PTR_AS(DictIterator, self)->tbl_iter;
    retval = YogTableIterator_current_key(env, tbl_iter);

    RETURN(env, retval);
}

BOOL
YogDictIterator_next(YogEnv* env, YogVal self)
{
    SAVE_ARG(env, self);
    YogVal tbl_iter = YUNDEF;
    PUSH_LOCAL(env, tbl_iter);

    tbl_iter = PTR_AS(DictIterator, self)->tbl_iter;
    BOOL retval = YogTableIterator_next(env, tbl_iter);

    RETURN(env, retval);
}

YogVal
YogDict_get_iterator(YogEnv* env, YogVal self)
{
    SAVE_ARG(env, self);
    YogVal iter = YUNDEF;
    YogVal tbl_iter = YUNDEF;
    PUSH_LOCALS2(env, iter, tbl_iter);

    tbl_iter = YogTable_get_iterator(env, PTR_AS(YogDict, self)->tbl);

    iter = ALLOC_OBJ(env, DictIterator_keep_children, NULL, DictIterator);
    PTR_AS(DictIterator, iter)->tbl_iter = tbl_iter;

    RETURN(env, iter);
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
    DEFINE_METHOD("+", add);
    DEFINE_METHOD("[]", subscript);
    DEFINE_METHOD("[]=", subscript_assign);
#undef DEFINE_METHOD
    YogKlass_define_property(env, klass, "size", get_size, NULL);

    RETURN(env, klass);
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
