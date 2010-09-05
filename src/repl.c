#include "yog/config.h"
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
print_prompt2()
{
    printf("... ");
}

static BOOL
eval(YogEnv* env, YogHandle* pkg, YogVal src)
{
    SAVE_ARG(env, src);
    YogVal stmts = YUNDEF;
    YogVal code = YUNDEF;
    PUSH_LOCALS2(env, stmts, code);

    YogJmpBuf jmpbuf;
    int_t status = setjmp(jmpbuf.buf);
    if (status != 0) {
        YogError_print_stacktrace(env);
        RETURN(env, TRUE);
    }
    INIT_JMPBUF(env, jmpbuf);
    PUSH_JMPBUF(env->thread, jmpbuf);

    stmts = YogParser_parse(env, src);
    if (!IS_PTR(stmts)) {
        POP_JMPBUF(env);
        RETURN(env, FALSE);
    }
    code = YogCompiler_compile_interactive(env, stmts);
    YogEval_eval_package(env, pkg, code);

    POP_JMPBUF(env);
    RETURN(env, TRUE);
}

void
YogRepl_do(YogEnv* env)
{
    SAVE_LOCALS(env);
    YogVal src = YUNDEF;
    YogVal enc = YUNDEF;
    PUSH_LOCALS2(env, src, enc);

    YogHandle* pkg = VAL2HDL(env, YogPackage_new(env));
    YogVal s = YogString_from_string(env, MAIN_MODULE_NAME);
    YogHandle* name = VAL2HDL(env, s);
    YogVM_register_package(env, env->vm, name, pkg);
    src = YogString_new(env);
    print_prompt();
    char buffer[4096];
    while (fgets(buffer, array_sizeof(buffer), stdin) != NULL) {
        YogString_append_string(env, src, buffer);
        if (eval(env, pkg, src)) {
            YogString_clear(env, src);
            print_prompt();
            continue;
        }
        print_prompt2();
    }

    RETURN_VOID(env);
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
