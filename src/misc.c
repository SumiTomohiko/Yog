#include "yog/config.h"
#include "yog/compile.h"
#include "yog/error.h"
#include "yog/eval.h"
#include "yog/handle.h"
#include "yog/parser.h"
#include "yog/sprintf.h"
#include "yog/string.h"
#include "yog/yog.h"

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
YogMisc_eval_source(YogEnv* env, YogVal obj, const char* src)
{
    SAVE_ARG(env, obj);
    YogVal s = YUNDEF;
    YogVal stmts = YUNDEF;
    YogVal code = YUNDEF;
    PUSH_LOCALS3(env, s, stmts, code);

    s = YogString_from_str(env, src);
    stmts = YogParser_parse(env, s);
    code = YogCompiler_compile_package(env, "builtin", stmts);
    YogEval_eval_package(env, obj, code);

    RETURN_VOID(env);
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
