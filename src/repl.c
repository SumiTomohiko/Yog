#include <stdio.h>
#include "yog/compile.h"
#include "yog/encoding.h"
#include "yog/eval.h"
#include "yog/package.h"
#include "yog/parser.h"
#include "yog/string.h"
#include "yog/thread.h"
#include "yog/vm.h"
#include "yog/yog.h"

static void
print_prompt()
{
    printf(">>> ");
}

void
YogRepl_do(YogEnv* env)
{
    SAVE_LOCALS(env);
    YogVal stmts = YUNDEF;
    YogVal code = YUNDEF;
    YogVal pkg = YUNDEF;
    YogVal src = YUNDEF;
    YogVal enc = YUNDEF;
    PUSH_LOCALS5(env, stmts, code, pkg, src, enc);

    pkg = YogPackage_new(env);
    YogVM_register_package(env, env->vm, MAIN_MODULE_NAME, pkg);

    src = YogString_new(env);
    enc = YogEncoding_get_default(env);
    PTR_AS(YogString, src)->encoding = enc;

    print_prompt();

    char buffer[4096];
    while (fgets(buffer, array_sizeof(buffer), stdin) != NULL) {
        YogString_add(env, src, buffer);

        stmts = YogParser_parse(env, src);
        if (!IS_PTR(stmts)) {
            continue;
        }
        code = YogCompiler_compile_module(env, MAIN_MODULE_NAME, stmts);

        PTR_AS(YogPackage, pkg)->code = code;
        YogEval_eval_package(env, pkg);

        YogString_clear(env, src);
        print_prompt();
    }
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
