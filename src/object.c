#include "yog/array.h"
#include "yog/class.h"
#include "yog/error.h"
#include "yog/eval.h"
#include "yog/frame.h"
#include "yog/gc.h"
#include "yog/get_args.h"
#include "yog/misc.h"
#include "yog/object.h"
#include "yog/sprintf.h"
#include "yog/string.h"
#include "yog/sysdeps.h"
#include "yog/table.h"
#include "yog/thread.h"
#include "yog/vm.h"
#include "yog/yog.h"

YogVal
YogObj_get_attr(YogEnv* env, YogVal obj, ID name)
{
    if (!IS_PTR(PTR_AS(YogObj, obj)->attrs)) {
        return YUNDEF;
    }

    YogVal key = ID2VAL(name);
    YogVal attr = YNIL;
    if (YogTable_lookup_sym(env, PTR_AS(YogObj, obj)->attrs, key, &attr)) {
        return attr;
    }
    return YUNDEF;
}

void
YogObj_set_attr_id(YogEnv* env, YogVal obj, ID name, YogVal val)
{
    SAVE_LOCALS(env);
    PUSH_LOCALS2(env, obj, val);

    YogVal key = ID2VAL(name);

    if (!IS_PTR(PTR_AS(YogObj, obj)->attrs)) {
        YogVal attrs = YogTable_create_symbol_table(env);
        YogGC_UPDATE_PTR(env, PTR_AS(YogObj, obj), attrs, attrs);
    }

    YogTable_insert(env, PTR_AS(YogObj, obj)->attrs, key, val);

    RETURN_VOID(env);
}

void
YogObj_set_attr(YogEnv* env, YogVal obj, const char* name, YogVal val)
{
    SAVE_ARGS2(env, obj, val);

    ID id = YogVM_intern(env, env->vm, name);
    YogObj_set_attr_id(env, obj, id, val);

    RETURN_VOID(env);
}

void
YogBasicObj_init(YogEnv* env, YogVal obj, type_t type, uint_t flags, YogVal klass)
{
    YogThread_issue_object_id(env, env->thread, obj);
    PTR_AS(YogBasicObj, obj)->type = type;
    PTR_AS(YogBasicObj, obj)->flags = flags;
    YogGC_UPDATE_PTR(env, PTR_AS(YogBasicObj, obj), klass, klass);
}

void
YogObj_init(YogEnv* env, YogVal obj, uint_t type, uint_t flags, YogVal klass)
{
    YogBasicObj_init(env, obj, type, flags | HAS_ATTRS, klass);
    PTR_AS(YogObj, obj)->attrs = YUNDEF;
}

void
YogBasicObj_keep_children(YogEnv* env, void* ptr, ObjectKeeper keeper, void* heap)
{
    YogBasicObj* obj = PTR_AS(YogBasicObj, ptr);
    YogGC_KEEP(env, obj, klass, keeper, heap);
}

void
YogObj_keep_children(YogEnv* env, void* ptr, ObjectKeeper keeper, void* heap)
{
    YogBasicObj_keep_children(env, ptr, keeper, heap);

    YogObj* obj = PTR_AS(YogObj, ptr);
    YogGC_KEEP(env, obj, attrs, keeper, heap);
}

YogVal
YogObj_alloc(YogEnv* env, YogVal klass)
{
    SAVE_ARG(env, klass);

    YogVal obj = ALLOC_OBJ(env, YogObj_keep_children, NULL, YogObj);
    YogObj_init(env, obj, TYPE_OBJ, 0, klass);

    RETURN(env, obj);
}

YogVal
YogObj_new(YogEnv* env, YogVal klass)
{
    return YogObj_alloc(env, klass);
}

static YogVal
init(YogEnv* env, YogVal self, YogVal pkg, YogVal args, YogVal kw, YogVal block)
{
    return YNIL;
}

void
YogObj_class_init(YogEnv* env, YogVal klass, YogVal pkg)
{
    SAVE_ARGS2(env, klass, pkg);
    YogClass_define_method(env, klass, pkg, "init", init);
    RETURN_VOID(env);
}

YogVal
YogBasicObj_to_s(YogEnv* env, YogVal self)
{
    SAVE_ARG(env, self);
    YogVal s = YUNDEF;
    PUSH_LOCAL(env, s);
    char id_upper[9];
    YogSysdeps_snprintf(id_upper, array_sizeof(id_upper), "%08zx", BASIC_OBJ(self)->id_upper);
    char id_lower[9];
    YogSysdeps_snprintf(id_lower, array_sizeof(id_lower), "%08zx", BASIC_OBJ(self)->id_lower);
    s = YogSprintf_sprintf(env, "<%C %s%s>", self, id_upper, id_lower);
    RETURN(env, s);
}

static YogVal
to_s(YogEnv* env, YogVal self, YogVal pkg, YogVal args, YogVal kw, YogVal block)
{
    SAVE_ARGS5(env, self, pkg, args, kw, block);
    YogVal s = YUNDEF;
    PUSH_LOCAL(env, s);
    YogCArg params[] = { { NULL, NULL } };
    YogGetArgs_parse_args(env, "to_s", params, args, kw);

    s = YogBasicObj_to_s(env, self);
    RETURN(env, s);
}

static YogVal
get_class(YogEnv* env, YogVal self, YogVal pkg, YogVal args, YogVal kw, YogVal block)
{
    SAVE_ARGS5(env, self, pkg, args, kw, block);
    YogVal klass = YUNDEF;
    PUSH_LOCAL(env, klass);

    YogCArg params[] = { { NULL, NULL } };
    YogGetArgs_parse_args(env, "get_class", params, args, kw);

    klass = YogVal_get_class(env, self);

    RETURN(env, klass);
}

static YogVal
kind_of(YogEnv* env, YogVal self, YogVal pkg, YogVal args, YogVal kw, YogVal block)
{
    SAVE_ARGS5(env, self, pkg, args, kw, block);
    YogVal c = YUNDEF;
    YogVal klass = YUNDEF;
    PUSH_LOCALS2(env, c, klass);
    YogCArg params[] = { { "c", &c }, { NULL, NULL } };
    YogGetArgs_parse_args(env, "kind_of?", params, args, kw);

    klass = YogVal_get_class(env, self);
    while (IS_PTR(klass)) {
        if (klass == c) {
            RETURN(env, YTRUE);
        }
        klass = PTR_AS(YogClass, klass)->super;
    }

    RETURN(env, YFALSE);
}

static YogVal
hash(YogEnv* env, YogVal self, YogVal pkg, YogVal args, YogVal kw, YogVal block)
{
    YOG_ASSERT(env, IS_PTR(self), "self is not pointer (0x%08x)", self);

    SAVE_ARGS5(env, self, pkg, args, kw, block);
    YogVal retval = YUNDEF;
    PUSH_LOCAL(env, retval);

    YogCArg params[] = { { NULL, NULL } };
    YogGetArgs_parse_args(env, "hash", params, args, kw);

    retval = INT2VAL(PTR_AS(YogBasicObj, self)->id_upper + PTR_AS(YogBasicObj, self)->id_lower);

    RETURN(env, retval);
}

static YogVal
not_equal(YogEnv* env, YogVal self, YogVal pkg, YogVal args, YogVal kw, YogVal block)
{
    YOG_ASSERT(env, !IS_UNDEF(self), "self is not pointer (0x%08x)", self);

    SAVE_ARGS5(env, self, pkg, args, kw, block);
    YogVal b = YUNDEF;
    YogVal obj = YUNDEF;
    PUSH_LOCALS2(env, b, obj);

    YogCArg params[] = { { "obj", &obj }, { NULL, NULL } };
    YogGetArgs_parse_args(env, "!=", params, args, kw);

    b = YogEval_call_method1(env, self, "==", obj);
    if (YOG_TEST(b)) {
        RETURN(env, YFALSE);
    }

    RETURN(env, YTRUE);
}

static YogVal
equal(YogEnv* env, YogVal self, YogVal pkg, YogVal args, YogVal kw, YogVal block)
{
    SAVE_ARGS5(env, self, pkg, args, kw, block);
    YogVal obj = YUNDEF;
    PUSH_LOCAL(env, obj);

    YogCArg params[] = { { "obj", &obj }, { NULL, NULL } };
    YogGetArgs_parse_args(env, "==", params, args, kw);

    if (self == obj) {
        RETURN(env, YTRUE);
    }

    RETURN(env, YFALSE);
}

void
YogObject_eval_builtin_script(YogEnv* env, YogVal klass)
{
    const char* src =
#include "object.inc"
    ;
    YogMisc_eval_source(env, VAL2HDL(env, klass), src);
}

static YogVal
get_attr(YogEnv* env, YogVal self, YogVal pkg, YogVal args, YogVal kw, YogVal block)
{
    SAVE_ARGS5(env, self, pkg, args, kw, block);
    YogVal name = YUNDEF;
    YogVal val = YUNDEF;
    PUSH_LOCALS2(env, name, val);

    YogCArg params[] = { { "name", &name}, { NULL, NULL } };
    YogGetArgs_parse_args(env, "get_attr", params, args, kw);

    ID id;
    if (IS_PTR(name) && (BASIC_OBJ_TYPE(name) == TYPE_STRING)) {
        id = YogString_intern(env, name);
    }
    else if (IS_SYMBOL(name)) {
        id = VAL2ID(name);
    }
    else {
        const char* msg = "Attribute name must be String or Symbol";
        YogError_raise_TypeError(env, msg);
        /* NOTREACHED */
        /**
         * gcc complains "'id' may be used uninitialized in this function"
         * without the following assignment.
         */
        id = 0;
    }

    val = YogVal_get_attr(env, self, id);
    if (IS_UNDEF(val)) {
        YogError_raise_AttributeError(env, "An instance has no attribute");
    }

    RETURN(env, val);
}

void
YogObject_boot(YogEnv* env, YogVal cObject, YogVal pkg)
{
    SAVE_ARGS2(env, cObject, pkg);

#define DEFINE_METHOD(name, f)  do { \
    YogClass_define_method(env, cObject, pkg, name, f); \
} while (0)
    DEFINE_METHOD("!=", not_equal);
    DEFINE_METHOD("==", equal);
    DEFINE_METHOD("get_attr", get_attr);
    DEFINE_METHOD("hash", hash);
    DEFINE_METHOD("kind_of?", kind_of);
    DEFINE_METHOD("to_s", to_s);
#undef DEFINE_METHOD
#define DEFINE_PROP(name, getter, setter)   do { \
    YogClass_define_property(env, cObject, pkg, (name), (getter), (setter)); \
} while (0)
    DEFINE_PROP("class", get_class, NULL);
#undef DEFINE_PROP

    RETURN_VOID(env);
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
