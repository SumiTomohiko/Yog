#include "yog/array.h"
#include "yog/callable.h"
#include "yog/class.h"
#include "yog/dict.h"
#include "yog/error.h"
#include "yog/get_args.h"
#include "yog/misc.h"
#include "yog/object.h"
#include "yog/table.h"
#include "yog/vm.h"
#include "yog/yog.h"

#define CHECK_SELF_TYPE(env, self)  do { \
    if (!IS_PTR(self) || (BASIC_OBJ_TYPE(self) != TYPE_DICT)) { \
        YogError_raise_TypeError((env), "self must be Dict"); \
    } \
} while (0)

struct DictIterator {
    YogVal tbl_iter;
};

typedef struct DictIterator DictIterator;

YogVal
YogDict_add(YogEnv* env, YogVal self, YogVal dict)
{
    SAVE_ARGS2(env, self, dict);
    YogVal iter = YUNDEF;
    YogVal key = YUNDEF;
    YogVal value = YUNDEF;
    PUSH_LOCALS3(env, iter, key, value);

    iter = YogDict_get_iterator(env, dict);
    while (YogDictIterator_next(env, iter)) {
        key = YogDictIterator_current_key(env, iter);
        value = YogDictIterator_current_value(env, iter);
        YogDict_set(env, self, key, value);
    }

    RETURN(env, self);
}

static YogVal
each(YogEnv* env, YogVal self, YogVal pkg, YogVal args, YogVal kw, YogVal block)
{
    SAVE_ARGS5(env, self, pkg, args, kw, block);
    YogVal iter = YUNDEF;
    PUSH_LOCAL(env, iter);
    YogVal params[] = { YUNDEF, YUNDEF };
    PUSH_LOCALSX(env, array_sizeof(params), params);

    YogCArg cargs[] = { { NULL, NULL } };
    YogGetArgs_parse_args(env, "each", cargs, args, kw);
    CHECK_SELF_TYPE(env, self);

    iter = YogDict_get_iterator(env, self);
    while (YogDictIterator_next(env, iter)) {
        params[0] = YogDictIterator_current_key(env, iter);
        params[1] = YogDictIterator_current_value(env, iter);
        YogCallable_call(env, block, array_sizeof(params), params);
    }

    RETURN(env, self);
}

static YogVal
add(YogEnv* env, YogVal self, YogVal pkg, YogVal args, YogVal kw, YogVal block)
{
    SAVE_ARGS5(env, self, pkg, args, kw, block);
    YogVal right = YUNDEF;
    YogVal dict = YUNDEF;
    PUSH_LOCALS2(env, right, dict);

    YogCArg params[] = { { "d", &right }, { NULL, NULL } };
    YogGetArgs_parse_args(env, "+", params, args, kw);
    CHECK_SELF_TYPE(env, self);
    if (!IS_PTR(right) || (BASIC_OBJ_TYPE(right) != TYPE_DICT)) {
        YogError_raise_TypeError(env, "operand must be Dict");
    }

    dict = YogDict_new(env);
    YogDict_add(env, dict, self);
    YogDict_add(env, dict, right);

    RETURN(env, dict);
}

YogVal
YogDict_get(YogEnv* env, YogVal self, YogVal key)
{
    SAVE_ARGS2(env, self, key);
    YogVal val = YUNDEF;
    PUSH_LOCAL(env, val);

    if (YogTable_lookup(env, PTR_AS(YogDict, self)->tbl, key, &val)) {
        RETURN(env, val);
    }

    RETURN(env, YUNDEF);
}

BOOL
YogDict_include(YogEnv* env, YogVal self, YogVal key)
{
    return IS_UNDEF(YogDict_get(env, self, key)) ? FALSE : TRUE;
}

static YogVal
subscript(YogEnv* env, YogVal self, YogVal pkg, YogVal args, YogVal kw, YogVal block)
{
    SAVE_ARGS5(env, self, pkg, args, kw, block);
    YogVal key = YUNDEF;
    YogVal value = YUNDEF;
    PUSH_LOCALS2(env, key, value);

    YogCArg params[] = { { "key", &key }, { NULL, NULL } };
    YogGetArgs_parse_args(env, "[]", params, args, kw);
    CHECK_SELF_TYPE(env, self);

    if (YogTable_lookup(env, PTR_AS(YogDict, self)->tbl, key, &value)) {
        RETURN(env, value);
    }

    YogError_raise_KeyError(env, "not found");

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
subscript_assign(YogEnv* env, YogVal self, YogVal pkg, YogVal args, YogVal kw, YogVal block)
{
    SAVE_ARGS5(env, self, pkg, args, kw, block);
    YogVal key = YUNDEF;
    YogVal value = YUNDEF;
    PUSH_LOCALS2(env, key, value);

    YogCArg params[] = { { "key", &key }, { "value", &value }, { NULL, NULL } };
    YogGetArgs_parse_args(env, "[]=", params, args, kw);
    CHECK_SELF_TYPE(env, self);

    YogDict_set(env, self, key, value);

    RETURN(env, self);
}

static void
init(YogEnv* env, YogVal self, YogVal klass)
{
    SAVE_ARGS2(env, self, klass);
    YogVal tbl = YUNDEF;
    PUSH_LOCAL(env, tbl);

    YogBasicObj_init(env, self, TYPE_DICT, 0, klass);
    tbl = YogTable_new_val_table(env);
    YogGC_UPDATE_PTR(env, PTR_AS(YogDict, self), tbl, tbl);

    RETURN_VOID(env);
}

static void
keep_children(YogEnv* env, void* ptr, ObjectKeeper keeper, void* heap)
{
    YogBasicObj_keep_children(env, ptr, keeper, heap);

    YogDict* dict = PTR_AS(YogDict, ptr);
#define KEEP(member)    YogGC_keep(env, &dict->member, keeper, heap)
    KEEP(tbl);
#undef KEEP
}

YogVal
YogDict_alloc(YogEnv* env, YogVal klass)
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
    return YogDict_alloc(env, env->vm->cDict);
}

uint_t
YogDict_size(YogEnv* env, YogVal self)
{
    SAVE_ARG(env, self);
    YogVal tbl = YUNDEF;
    PUSH_LOCAL(env, tbl);

    tbl = PTR_AS(YogDict, self)->tbl;
    int_t size = YogTable_size(env, tbl);

    RETURN(env, size);
}

static YogVal
get_size(YogEnv* env, YogVal self, YogVal pkg, YogVal args, YogVal kw, YogVal block)
{
    SAVE_ARGS5(env, self, pkg, args, kw, block);
    YogVal retval = YUNDEF;
    PUSH_LOCAL(env, retval);

    YogCArg params[] = { { NULL, NULL } };
    YogGetArgs_parse_args(env, "get_size", params, args, kw);

    int_t size = YogDict_size(env, self);
    retval = YogVal_from_int(env, size);

    RETURN(env, retval);
}

static void
DictIterator_keep_children(YogEnv* env, void* ptr, ObjectKeeper keeper, void* heap)
{
    DictIterator* iter = PTR_AS(DictIterator, ptr);
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
    YogGC_UPDATE_PTR(env, PTR_AS(DictIterator, iter), tbl_iter, tbl_iter);

    RETURN(env, iter);
}

void
YogDict_eval_builtin_script(YogEnv* env, YogVal klass)
{
#if !defined(MINIYOG)
    const char* src =
#   include "dict.inc"
    ;
    YogMisc_eval_source(env, klass, src);
#endif
}

void
YogDict_define_classes(YogEnv* env, YogVal pkg)
{
    SAVE_ARG(env, pkg);
    YogVal cDict = YUNDEF;
    PUSH_LOCAL(env, cDict);
    YogVM* vm = env->vm;

    cDict = YogClass_new(env, "Dict", vm->cObject);
    YogClass_define_allocator(env, cDict, YogDict_alloc);
#define DEFINE_METHOD(name, f)  do { \
    YogClass_define_method(env, cDict, pkg, (name), (f)); \
} while (0)
    DEFINE_METHOD("+", add);
    DEFINE_METHOD("[]", subscript);
    DEFINE_METHOD("[]=", subscript_assign);
    DEFINE_METHOD("each", each);
#undef DEFINE_METHOD
#define DEFINE_PROP(name, getter, setter)   do { \
    YogClass_define_property(env, cDict, pkg, (name), (getter), (setter)); \
} while (0)
    DEFINE_PROP("size", get_size, NULL);
#undef DEFINE_PROP
    vm->cDict = cDict;

    RETURN_VOID(env);
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
