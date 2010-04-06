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
eval(YogEnv* env, YogVal pkg, YogVal src)
{
    SAVE_ARGS2(env, pkg, src);
    YogVal stmts = YUNDEF;
    YogVal code = YUNDEF;
    YogVal cur_frame = YUNDEF;
    PUSH_LOCALS3(env, stmts, code, cur_frame);

    SAVE_CURRENT_STAT(env, repl);

    YogJmpBuf jmpbuf;
    int_t status;
    if ((status = setjmp(jmpbuf.buf)) == 0) {
        PUSH_JMPBUF(env->thread, jmpbuf);

        stmts = YogParser_parse(env, src);
        if (!IS_PTR(stmts)) {
            RETURN(env, FALSE);
        }
        code = YogCompiler_compile_interactive(env, stmts);
        YogEval_eval_package(env, pkg, code);

        POP_JMPBUF(env);
    }
    else {
        RESTORE_STAT(env, repl);
        YogError_print_stacktrace(env);
    }

    RETURN(env, TRUE);
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
    YogGC_UPDATE_PTR(PTR_AS(YogString, src), encoding, enc);

    print_prompt();

    char buffer[4096];
    while (fgets(buffer, array_sizeof(buffer), stdin) != NULL) {
        YogString_add_cstr(env, src, buffer);
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
