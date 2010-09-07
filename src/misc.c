#include "yog/config.h"
#include "yog/binary.h"
#include "yog/compile.h"
#include "yog/error.h"
#include "yog/eval.h"
#include "yog/handle.h"
#include "yog/parser.h"
#include "yog/sprintf.h"
#include "yog/string.h"
#include "yog/sysdeps.h"
#include "yog/yog.h"

void
YogMisc_check_string(YogEnv* env, YogHandle* val, const char* name)
{
    if (IS_PTR(HDL2VAL(val)) && (BASIC_OBJ_TYPE(HDL2VAL(val)) == TYPE_STRING)) {
        return;
    }
    YogError_raise_TypeError(env, "%s must be String, not %C", name, val);
}

static YogHandle*
get_so_path(YogEnv* env, YogHandle* filename)
{
    if (0 <= YogString_find_char(env, HDL2VAL(filename), 0, PATH_SEPARATOR)) {
        return filename;
    }
    uint_t n = STRING_SIZE(HDL2VAL(filename)) + 2;
    YogHandle* path = VAL2HDL(env, YogString_of_size(env, n));
    YogString_append_string(env, HDL2VAL(path), "./");
    YogString_append(env, HDL2VAL(path), HDL2VAL(filename));
    return path;
}

LIB_HANDLE
YogMisc_load_lib(YogEnv* env, YogHandle* filename)
{
    YogHandle* path = get_so_path(env, filename);
    YogVal bin = YogString_to_bin_in_default_encoding(env, path);
    YogSysdeps_dlerror();
    return YogSysdeps_open_lib(BINARY_CSTR(bin));
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
