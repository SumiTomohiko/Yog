#include "yog/compile.h"
#include "yog/eval.h"
#include "yog/parser.h"
#include "yog/string.h"
#include "yog/thread.h"
#include "yog/yog.h"

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
