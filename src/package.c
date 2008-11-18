#include <stdarg.h>
#include "yog/yog.h"

void 
YogPackage_define_method(YogEnv* env, YogPackage* pkg, const char* name, void* f, unsigned int blockargc, unsigned int varargc, unsigned int kwargc, unsigned int required_argc, ...)
{
    va_list ap;
    va_start(ap, required_argc);
    YogBuiltinFunction* builtin_f = YogBuiltinFunction_new(env, name, f, blockargc, varargc, kwargc, required_argc, ap);
    va_end(ap);

    YogBuiltinBoundMethod* method = YogBuiltinBoundMethod_new(env);
    method->self = OBJ2VAL(pkg);
    method->f = builtin_f;

    YogVal val = OBJ2VAL(method);
    YogObj_set_attr(env, pkg, name, val);
}

static YogBasicObj* 
allocate(YogEnv* env, YogKlass* klass) 
{
    YogPackage* pkg = ALLOC_OBJ(env, YogObj_gc_children, YogPackage);
    YogPackage_init(env, pkg, 0, klass);

    return YOGBASICOBJ(pkg);
}

YogKlass* 
YogPackage_klass_new(YogEnv* env) 
{
    YogKlass* klass = YogKlass_new(env, allocate, "Package", ENV_VM(env)->cObject);
    return klass;
}

YogPackage* 
YogPackage_new(YogEnv* env) 
{
    YogPackage* pkg = (YogPackage*)allocate(env, ENV_VM(env)->cPackage);
    pkg->attrs = YogTable_new_symbol_table(env);

    return pkg;
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
