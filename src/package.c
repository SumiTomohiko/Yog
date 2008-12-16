#include <stdarg.h>
#include "yog/function.h"
#include "yog/method.h"
#include "yog/st.h"
#include "yog/yog.h"

void 
YogPackage_define_method(YogEnv* env, YogPackage* pkg, const char* name, void* f, unsigned int blockargc, unsigned int varargc, unsigned int kwargc, unsigned int required_argc, ...)
{
    FRAME_DECL_LOCAL(env, pkg_idx, OBJ2VAL(pkg));

    ID func_name = INTERN(name);

    va_list ap;
    va_start(ap, required_argc);
    YogBuiltinFunction* builtin_f = YogBuiltinFunction_new(env, f, INVALID_ID, func_name, blockargc, varargc, kwargc, required_argc, ap);
    va_end(ap);
    FRAME_DECL_LOCAL(env, builtin_f_idx, OBJ2VAL(builtin_f));

    YogBuiltinBoundMethod* method = YogBuiltinBoundMethod_new(env);
    FRAME_LOCAL_OBJ(env, pkg, YogPackage, pkg_idx);
    method->self = OBJ2VAL(pkg);
    FRAME_LOCAL_PTR(env, builtin_f, builtin_f_idx);
    method->f = builtin_f;

    YogVal val = OBJ2VAL(method);
    YogObj_set_attr_id(env, YOGOBJ(pkg), func_name, val);
}

static void 
YogPackage_keep_children(YogEnv* env, void* ptr, ObjectKeeper keeper) 
{
    YogObj_keep_children(env, ptr, keeper);

    YogPackage* pkg = ptr;
    pkg->code = (*keeper)(env, pkg->code);
}

static YogBasicObj* 
allocate(YogEnv* env, YogKlass* klass) 
{
    YogPackage* pkg = ALLOC_OBJ(env, YogPackage_keep_children, NULL, YogPackage);
    YogObj_init(env, YOGOBJ(pkg), 0, ENV_VM(env)->cPackage);
    pkg->code = NULL;
    FRAME_DECL_LOCAL(env, pkg_idx, OBJ2VAL(pkg));

    YogTable* attrs = YogTable_new_symbol_table(env);
#define UPDATE_PTR  FRAME_LOCAL_OBJ(env, pkg, YogPackage, pkg_idx)
    UPDATE_PTR;
    YOGOBJ(pkg)->attrs = attrs;

    UPDATE_PTR;
    return YOGBASICOBJ(pkg);
#undef UPDATE_PTR
}

YogKlass* 
YogPackage_klass_new(YogEnv* env) 
{
    YogKlass* klass = YogKlass_new(env, "Package", ENV_VM(env)->cObject);
    FRAME_DECL_LOCAL(env, klass_idx, OBJ2VAL(klass));

#define UPDATE_PTR  FRAME_LOCAL_OBJ(env, klass, YogKlass, klass_idx)
    UPDATE_PTR;
    YogKlass_define_allocator(env, klass, allocate);

    UPDATE_PTR;
    return klass;
#undef UPDATE_PTR
}

YogPackage* 
YogPackage_new(YogEnv* env) 
{
    YogPackage* pkg = (YogPackage*)allocate(env, ENV_VM(env)->cPackage);
    return pkg;
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
