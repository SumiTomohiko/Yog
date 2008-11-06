#include <stdarg.h>
#include "yog/yog.h"

void 
YogPkg_define_method(YogEnv* env, YogPkg* pkg, const char* name, void* f, unsigned int blockargc, unsigned int varargc, unsigned int kwargc, unsigned int required_argc, ...)
{
    va_list ap;
    va_start(ap, required_argc);
    YogBuiltinFunction* builtin_f = YogBuiltinFunction_new(env, name, f, blockargc, varargc, kwargc, required_argc, ap);
    va_end(ap);

    YogBuiltinBoundMethod* method = YogBuiltinBoundMethod_new(env);
    method->self = YogVal_obj(YOGBASICOBJ(pkg));
    method->f = builtin_f;

    YogVal val = YogVal_obj(YOGBASICOBJ(method));
    YogObj_set_attr(env, pkg, name, val);
}

YogKlass* 
YogPkg_klass_new(YogEnv* env) 
{
    YogKlass* klass = YogKlass_new(env, "Package", ENV_VM(env)->obj_klass);
    return klass;
}

YogPkg* 
YogPkg_new(YogEnv* env) 
{
    YogPkg* pkg = ALLOC_OBJ(env, YogObj_gc_children, YogPkg);
    YogObj_init(env, pkg, ENV_VM(env)->pkg_klass);
    pkg->attrs = YogTable_new_symbol_table(env);

    return pkg;
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
