#include "yog/config.h"
#include "yog/binary.h"
#include "yog/compile.h"
#include "yog/encoding.h"
#include "yog/error.h"
#include "yog/eval.h"
#include "yog/handle.h"
#include "yog/parser.h"
#include "yog/sprintf.h"
#include "yog/string.h"
#include "yog/sysdeps.h"
#include "yog/yog.h"

void
YogMisc_raise_TypeError(YogEnv* env, YogVal val, const char* name, const char* expected)
{
    YogError_raise_TypeError(env, "%s must be %s, not %C", name, expected, val);
}

void
YogMisc_check_Encoding(YogEnv* env, YogHandle* val, const char* name)
{
    YogVal v = HDL2VAL(val);
    if (IS_PTR(v) && (BASIC_OBJ_TYPE(v) == TYPE_ENCODING)) {
        return;
    }
    YogMisc_raise_TypeError(env, v, name, "Encoding");
}

void
YogMisc_check_Fixnum(YogEnv* env, YogHandle* val, const char* name)
{
    YogVal v = HDL2VAL(val);
    if (IS_FIXNUM(v)) {
        return;
    }
    YogMisc_raise_TypeError(env, v, name, "Fixnum");
}

void
YogMisc_check_Fixnum_optional(YogEnv* env, YogHandle* val, const char* name)
{
    if (val == NULL) {
        return;
    }
    YogMisc_check_Fixnum(env, val, name);
}

void
YogMisc_check_String(YogEnv* env, YogHandle* val, const char* name)
{
    YogVal v = HDL2VAL(val);
    if (IS_PTR(v) && (BASIC_OBJ_TYPE(v) == TYPE_STRING)) {
        return;
    }
    YogMisc_raise_TypeError(env, v, name, "String");
}

void*
YogMisc_load_lib(YogEnv* env, YogHandle* filename)
{
    YogVal bin = YogString_to_bin_in_default_encoding(env, filename);
    dlerror();
    return dlopen(BINARY_CSTR(bin), RTLD_LAZY);
}

YogHandle*
YogMisc_format_method_id(YogEnv* env, ID class_name, ID func_name)
{
    if (class_name == INVALID_ID) {
        return VAL2HDL(env, YogSprintf_sprintf(env, "%I", func_name));
    }
    const char* fmt = "%I#%I";
    return VAL2HDL(env, YogSprintf_sprintf(env, fmt, class_name, func_name));
}

YogHandle*
YogMisc_format_method(YogEnv* env, YogHandle* class_name, YogHandle* func_name)
{
    if ((class_name == NULL) || (!IS_PTR(HDL2VAL(class_name)))) {
        return func_name;
    }
    YOG_ASSERT(env, BASIC_OBJ_TYPE(HDL2VAL(class_name)) == TYPE_STRING, "Class name is not String");
    YOG_ASSERT(env, BASIC_OBJ_TYPE(HDL2VAL(func_name)) == TYPE_STRING, "Function name is not String");
    YogVal s = YogSprintf_sprintf(env, "%S#%S", HDL2VAL(class_name), HDL2VAL(func_name));
    return YogHandle_REGISTER(env, s);
}

void
YogMisc_eval_source(YogEnv* env, YogHandle* obj, const char* src)
{
    YogHandle* name = VAL2HDL(env, YogString_from_string(env, "builtins"));
    YogVal s = YogString_from_string(env, src);
    YogVal stmts = YogParser_parse(env, s);
    YogVal code = YogCompiler_compile_package(env, name, stmts);
    YogEval_eval_package(env, obj, code);
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
