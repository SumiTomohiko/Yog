#include <setjmp.h>
#include <stdio.h>
#include "yog/compile.h"
#include "yog/encoding.h"
#include "yog/error.h"
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

static void
eval(YogEnv* env, YogVal pkg, YogVal src)
{
    SAVE_ARGS2(env, pkg, src);
    YogVal stmts = YUNDEF;
    YogVal code = YUNDEF;
    YogVal cur_frame = YUNDEF;
    PUSH_LOCALS3(env, stmts, code, cur_frame);

    STORE_CURRENT_STAT(env, repl);

    YogJmpBuf jmpbuf;
    int_t status;
    if ((status = setjmp(jmpbuf.buf)) == 0) {
        PUSH_JMPBUF(env->thread, jmpbuf);

        stmts = YogParser_parse(env, src);
        if (!IS_PTR(stmts)) {
            RETURN_VOID(env);
        }
        code = YogCompiler_compile_interactive(env, stmts);

        YogEval_eval_package(env, pkg, code);
    }
    else {
        LOAD_STAT(env, repl);
        YogError_print_stacktrace(env);
    }

    YogString_clear(env, src);
    print_prompt();

    RETURN_VOID(env);
}

void
YogRepl_do(YogEnv* env)
{
    SAVE_LOCALS(env);
    YogVal pkg = YUNDEF;
    YogVal src = YUNDEF;
    YogVal enc = YUNDEF;
    PUSH_LOCALS3(env, pkg, src, enc);

    pkg = YogPackage_new(env);
    YogVM_register_package(env, env->vm, MAIN_MODULE_NAME, pkg);

    src = YogString_new(env);
    enc = YogEncoding_get_default(env);
    PTR_AS(YogString, src)->encoding = enc;

    print_prompt();

    char buffer[4096];
    while (fgets(buffer, array_sizeof(buffer), stdin) != NULL) {
        YogString_add(env, src, buffer);
        eval(env, pkg, src);
    }

    RETURN_VOID(env);
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
