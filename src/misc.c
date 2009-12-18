#include <alloca.h>
#include <string.h>
#include "yog/compile.h"
#include "yog/dict.h"
#include "yog/error.h"
#include "yog/eval.h"
#include "yog/parser.h"
#include "yog/string.h"
#include "yog/vm.h"
#include "yog/yog.h"

static void
convert_arg(YogEnv* env, const char* type, YogVal val, void* dest)
{
    /* TODO */
    YOG_BUG(env, "unknown type \"%s\"", type);
}

void
YogMisc_parse_params(YogEnv* env, const char* func_name, YogCParam* params, YogVal args, YogVal kw)
{
    SAVE_ARGS2(env, args, kw);
    YogVal val_in_args = YUNDEF;
    YogVal val_in_kw = YUNDEF;
    YogVal vals = YUNDEF;
    YogVal val = YUNDEF;
    PUSH_LOCALS4(env, val_in_args, val_in_kw, vals, val);
    BOOL optional = FALSE;

    uint_t size = YogArray_size(env, args);
    YogCParam* param = params;
    uint_t i = 0;
    const char* param_name = param->name;
    while (param_name != NULL) {
        param_name = param->name;
        if ((strcmp(param_name, "*") == 0) || (strcmp(param_name, "**") == 0)) {
            break;
        }
        else if (strcmp(param_name, "|") == 0) {
            optional = TRUE;
            param++;
            i++;
            param_name = param->name;
            YOG_ASSERT(env, param_name != NULL, "invalid parameter format");
        }

        if (i < size) {
            val_in_args = YogArray_at(env, args, i);
        }
        else {
            val_in_args = YUNDEF;
        }
        ID name = YogVM_intern(env, env->vm, param_name);
        val_in_kw = YogDict_get(env, kw, ID2VAL(name));
        if (!IS_UNDEF(val_in_args)) {
            if (!IS_UNDEF(val_in_kw)) {
                YogError_raise_TypeError(env, "Argument given by name (\"%s\") and position (%u)", param_name, i + 1);
            }
            convert_arg(env, param->type, val_in_args, param->dest);
        }
        else if (!IS_UNDEF(val_in_kw)) {
            convert_arg(env, param->type, val_in_kw, param->dest);
        }
        else if (!optional) {
            YogError_raise_TypeError(env, "Required argument \"%s\" (position %u) not found", param_name, i + 1);
        }

        param++;
        i++;
        param_name = param->name;
    }
    if (param_name == NULL) {
        RETURN_VOID(env);
    }

    if (strcmp(param_name, "*") == 0) {
        vals = YogArray_new(env);
        while (i < size) {
            val = YogArray_at(env, args, i);
            YogArray_push(env, vals, val);
            i++;
        }
        *(YogVal*)param->dest = vals;
    }

    RETURN_VOID(env);
}

void
YogMisc_eval_source(YogEnv* env, YogVal obj, const char* src)
{
    SAVE_ARG(env, obj);
    YogVal s = YUNDEF;
    YogVal stmts = YUNDEF;
    YogVal code = YUNDEF;
    PUSH_LOCALS3(env, s, stmts, code);

    s = YogString_new_str(env, src);
    stmts = YogParser_parse(env, s);
    code = YogCompiler_compile_package(env, "builtin", stmts);
    YogEval_eval_package(env, obj, code);

    RETURN_VOID(env);
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
