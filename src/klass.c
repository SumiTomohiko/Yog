#include <stdarg.h>
#include "yog/error.h"
#include "yog/function.h"
#include "yog/method.h"
#include "yog/yog.h"

void 
YogKlass_define_method(YogEnv* env, YogKlass* klass, const char* name, void* f, unsigned int blockargc, unsigned int varargc, unsigned int kwargc, int required_argc, ...)
{
    FRAME_DECL_LOCAL(env, klass_idx, OBJ2VAL(klass));
    ID func_name = INTERN(name);

    va_list ap;
    va_start(ap, required_argc);
    FRAME_LOCAL_OBJ(env, klass, YogKlass, klass_idx);
    YogBuiltinFunction* builtin_f = YogBuiltinFunction_new(env, f, klass->name, func_name, blockargc, varargc, kwargc, required_argc, ap);
    FRAME_DECL_LOCAL(env, builtin_f_idx, PTR2VAL(builtin_f));
    va_end(ap);

    YogBuiltinUnboundMethod* method = YogBuiltinUnboundMethod_new(env);
    FRAME_LOCAL_PTR(env, builtin_f, builtin_f_idx);
    method->f = builtin_f;

    FRAME_LOCAL_OBJ(env, klass, YogKlass, klass_idx);
    YogVal val = OBJ2VAL(method);
    YogObj_set_attr_id(env, YOGOBJ(klass), func_name, val);
}

static void 
keep_children(YogEnv* env, void* ptr, ObjectKeeper keeper)
{
    YogObj_keep_children(env, ptr, keeper);

    YogKlass* klass = ptr;
    klass->super = (*keeper)(env, klass->super);
}

YogBasicObj* 
YogKlass_allocate(YogEnv* env, YogKlass* klass) 
{
    FRAME_DECL_LOCAL(env, klass_idx, OBJ2VAL(klass));
    FRAME_LOCAL_OBJ(env, klass, YogKlass, klass_idx);
    YogObj* obj = ALLOC_OBJ(env, keep_children, NULL, YogKlass);
    FRAME_LOCAL_OBJ(env, klass, YogKlass, klass_idx);
    YogObj_init(env, obj, 0, klass);

    return (YogBasicObj*)obj;
}

void 
YogKlass_define_allocator(YogEnv* env, YogKlass* klass, Allocator allocator) 
{
    klass->allocator = allocator;
}

YogKlass* 
YogKlass_new(YogEnv* env, const char* name, YogKlass* super) 
{
    FRAME_DECL_LOCAL(env, super_idx, OBJ2VAL(super));
    FRAME_LOCAL_OBJ(env, super, YogKlass, super_idx);
    YogKlass* klass = (YogKlass*)YogKlass_allocate(env, ENV_VM(env)->cKlass);
    FRAME_LOCAL_OBJ(env, super, YogKlass, super_idx);
    klass->allocator = NULL;
    klass->name = INVALID_ID;
    klass->super = super;
    FRAME_DECL_LOCAL(env, klass_idx, OBJ2VAL(klass));
    FRAME_LOCAL_OBJ(env, klass, YogKlass, klass_idx);

    if (name != NULL) {
        ID id = INTERN(name);
        FRAME_LOCAL_OBJ(env, klass, YogKlass, klass_idx);
        klass->name = id;
    }

    return klass;
}

static YogVal 
new_(YogEnv* env)
{
    YogVal self = SELF(env);
    YogVal blockarg = ARG(env, 0);
    YogArray* vararg = OBJ_AS(YogArray, ARG(env, 1));

    YogKlass* klass = OBJ_AS(YogKlass, self);
    Allocator allocator = klass->allocator;
    while (allocator == NULL) {
        klass = klass->super;
        if (klass == NULL) {
            YOG_ASSERT(env, FALSE, "Can't allocate object.");
        }
        allocator = klass->allocator;
    }

    klass = OBJ_AS(YogKlass, self);
    YogBasicObj* obj = (*allocator)(env, klass);
    YogVal val = OBJ2VAL(obj);
    YogThread_call_method(env, ENV_TH(env), val, "initialize", vararg->body->size, vararg->body->items);

    return val;
}

void 
YogKlass_klass_init(YogEnv* env, YogKlass* cKlass) 
{
    YogKlass_define_method(env, cKlass, "new", new_, 1, 1, 0, 0, "block", NULL);
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
