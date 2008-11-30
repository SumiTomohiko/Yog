#include <stdarg.h>
#include "yog/yog.h"

void 
YogPackage_define_method(YogEnv* env, YogPackage* pkg, const char* name, void* f, unsigned int blockargc, unsigned int varargc, unsigned int kwargc, unsigned int required_argc, ...)
{
    va_list ap;
    va_start(ap, required_argc);
    YogBuiltinFunction* builtin_f = YogBuiltinFunction_new(env, f, blockargc, varargc, kwargc, required_argc, ap);
    va_end(ap);

    YogBuiltinBoundMethod* method = YogBuiltinBoundMethod_new(env);
    method->self = OBJ2VAL(pkg);
    method->f = builtin_f;

    YogVal val = OBJ2VAL(method);
    YogObj_set_attr(env, YOGOBJ(pkg), name, val);
}

static void 
YogPackage_keep_children(YogEnv* env, void* ptr, ObjectKeeper keeper) 
{
    YogObj_keep_children(env, ptr, keeper);

    YogPackage* pkg = ptr;
    pkg->code = (*keeper)(env, pkg->code);
}

static void 
YogPackage_init(YogEnv* env, YogPackage* pkg) 
{
    YogObj_init(env, YOGOBJ(pkg), 0, ENV_VM(env)->cPackage);

    YOGOBJ(pkg)->attrs = YogTable_new_symbol_table(env);
    pkg->code = NULL;
}

static YogBasicObj* 
allocate(YogEnv* env, YogKlass* klass) 
{
    YogPackage* pkg = ALLOC_OBJ(env, YogPackage_keep_children, YogPackage);
    YogPackage_init(env, pkg);

    return YOGBASICOBJ(pkg);
}

YogKlass* 
YogPackage_klass_new(YogEnv* env) 
{
    YogKlass* klass = YogKlass_new(env, "Package", ENV_VM(env)->cObject);
    YogKlass_define_allocator(env, klass, allocate);
    return klass;
}

YogPackage* 
YogPackage_new(YogEnv* env) 
{
    YogPackage* pkg = (YogPackage*)allocate(env, ENV_VM(env)->cPackage);
    YogPackage_init(env, pkg);

    return pkg;
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
