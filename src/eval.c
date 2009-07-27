#include <setjmp.h>
#include <string.h>
#include "yog/binary.h"
#include "yog/block.h"
#include "yog/code.h"
#include "yog/compile.h"
#include "yog/error.h"
#include "yog/eval.h"
#include "yog/exception.h"
#include "yog/frame.h"
#include "yog/function.h"
#include "yog/function.h"
#include "yog/package.h"
#include "yog/parser.h"
#include "yog/string.h"
#include "yog/thread.h"
#include "yog/vm.h"
#include "yog/yog.h"

#if 0
#   define DEBUG(x)     x
#else
#   define DEBUG(x)
#endif
#define DUMP_CODE(code)  YogCode_dump(env, code)

#define CUR_FRAME   PTR_AS(YogThread, env->thread)->cur_frame

#define PUSH_FRAME(f)   do { \
    PTR_AS(YogFrame, (f))->prev = CUR_FRAME; \
    CUR_FRAME = (f); \
} while (0)

#define POP_FRAME()     do { \
    CUR_FRAME = PTR_AS(YogFrame, CUR_FRAME)->prev; \
} while (0)

static YogVal*
get_outer_vars_ptr(YogEnv* env, unsigned int level, unsigned int index)
{
    YogVal outer_vars = PTR_AS(YogScriptFrame, CUR_FRAME)->outer_vars;
    YOG_ASSERT(env, IS_PTR(outer_vars), "no outer variables");
    unsigned int depth = PTR_AS(YogOuterVars, outer_vars)->size;
    YOG_ASSERT(env, level <= depth, "invalid level");
    YogVal vars = PTR_AS(YogOuterVars, outer_vars)->items[level - 1];
    unsigned int size = YogValArray_size(env, vars);
    YOG_ASSERT(env, index < size, "invalid index");
    return &PTR_AS(YogValArray, vars)->items[index];
}

YogVal 
YogEval_call_method(YogEnv* env, YogVal receiver, const char* method, unsigned int argc, YogVal* args) 
{
    SAVE_ARG(env, receiver);

    ID id = YogVM_intern(env, env->vm, method);
    YogVal retval = YogEval_call_method_id(env, receiver, id, argc, args);

    RETURN(env, retval);
}

YogVal 
YogEval_call_method2(YogEnv* env, YogVal receiver, const char* method, unsigned int argc, YogVal* args, YogVal blockarg)
{
    SAVE_ARGS2(env, receiver, blockarg);

    ID id = YogVM_intern(env, env->vm, method);
    YogVal retval = YogEval_call_method_id2(env, receiver, id, argc, args, blockarg);

    RETURN(env, retval);
}

static YogVal
make_outer_vars(YogEnv* env, unsigned int depth)
{
    if (depth == 0) {
        return YNIL;
    }

    SAVE_LOCALS(env);

    YogVal outer_vars = YUNDEF;
    YogVal vars = YUNDEF;
    PUSH_LOCALS2(env, outer_vars, vars);

    outer_vars = YogOuterVars_new(env, depth);
    vars = PTR_AS(YogMethodFrame, CUR_FRAME)->vars;
    PTR_AS(YogOuterVars, outer_vars)->items[0] = vars;
    if (depth == 1) {
        RETURN(env, outer_vars);
    }

    void* dest = &PTR_AS(YogOuterVars, outer_vars)->items[1];
    vars = PTR_AS(YogScriptFrame, CUR_FRAME)->outer_vars;
    YOG_ASSERT(env, IS_PTR(vars), "vars is not pointer");
    void* src = &PTR_AS(YogOuterVars, vars)->items[0];
    size_t unit = sizeof(PTR_AS(YogOuterVars, vars)->items[0]);
    memcpy(dest, src, unit * (depth - 1));

    RETURN(env, outer_vars);
}

static void 
setup_script_function(YogEnv* env, YogVal f, YogVal code) 
{
    SAVE_ARGS2(env, f, code);

    PTR_AS(YogFunction, f)->code = code;
    PTR_AS(YogFunction, f)->globals = PTR_AS(YogScriptFrame, CUR_FRAME)->globals;

    unsigned int outer_size = PTR_AS(YogCode, code)->outer_size;
    YogVal outer_vars = make_outer_vars(env, outer_size);
    PTR_AS(YogFunction, f)->outer_vars = outer_vars;

    RETURN_VOID(env);
}

static void 
setup_script_frame(YogEnv* env, YogVal frame, YogVal code) 
{
    SAVE_ARGS2(env, frame, code);

#if 0
    printf("%s:%d setup_script_frame(env=%p, frame=%p, code=%p)\n", __FILE__, __LINE__, env, frame, code);
    YogCode_dump(env, code);
#endif

    unsigned int stack_size = PTR_AS(YogCode, code)->stack_size;
    YogVal stack = YogValArray_new(env, stack_size);

    SCRIPT_FRAME(frame)->pc = 0;
    MODIFY(env, SCRIPT_FRAME(frame)->code, code);
    MODIFY(env, SCRIPT_FRAME(frame)->stack, stack);

    RETURN_VOID(env);
}

static YogVal 
lookup_builtins(YogEnv* env, ID name) 
{
    YogVal builtins_name = ID2VAL(INTERN("builtins"));
    YogVal builtins = YUNDEF;
    YogVM* vm = env->vm;
    if (!YogTable_lookup(env, vm->pkgs, builtins_name, &builtins)) {
        YOG_ASSERT(env, FALSE, "Can't find builtins package.");
    }

    YogPackage* pkg = PTR_AS(YogPackage, builtins);
    YogVal key = ID2VAL(name);
    YogVal val = YUNDEF;
    if (!YogTable_lookup(env, YOGOBJ(pkg)->attrs, key, &val)) {
        return YUNDEF;
    }

    return val;
}

YogVal
YogEval_mainloop(YogEnv* env)
{
    SAVE_LOCALS(env);

#define POP_BUF()   do { \
    YogJmpBuf* prev = PTR_AS(YogThread, env->thread)->jmp_buf_list->prev; \
    PTR_AS(YogThread, env->thread)->jmp_buf_list = prev; \
} while (0)
#define PC          (SCRIPT_FRAME(CUR_FRAME)->pc)
#undef CODE
#define CODE        PTR_AS(YogCode, SCRIPT_FRAME(CUR_FRAME)->code)
    YogJmpBuf jmpbuf;
    int status = 0;
    if ((status = setjmp(jmpbuf.buf)) == 0) {
        PUSH_JMPBUF(env->thread, jmpbuf);
    }
    else {
        RESTORE_LOCALS(env);

        unsigned int i = 0;
        BOOL found = FALSE;
        if (PTR_AS(YogFrame, CUR_FRAME)->type != FRAME_C) {
            for (i = 0; i < CODE->exc_tbl_size; i++) {
                YogVal exc_tbl = CODE->exc_tbl;
                YogExceptionTableEntry* entry = &PTR_AS(YogExceptionTable, exc_tbl)->items[i];
                if ((entry->from <= PC) && (PC < entry->to)) {
                    PC = entry->target;
                    found = TRUE;
                    break;
                }
            }
        }
        if (!found) {
            POP_BUF();
            YogJmpBuf* list = PTR_AS(YogThread, env->thread)->jmp_buf_list;
            if (list != NULL) {
                longjmp(list->buf, status);
            }

            YogError_print_stacktrace(env);

            RETURN(env, INT2VAL(-1));
        }
    }

#if 0
    YogCode_dump(env, (YogVal)(CODE));
#endif

    while (PC < PTR_AS(YogByteArray, CODE->insts)->size) {
#define ENV             (env)
#define VM              (ENV_VM(ENV))
#define POP()           (YogScriptFrame_pop_stack(env, SCRIPT_FRAME(CUR_FRAME)))
#define CONSTS(index)   (YogValArray_at(env, CODE->consts, index))
#define THREAD          (env->thread)
#define JUMP(m)         PC = m;
#define POP_ARGS(args, kwargs, blockarg, vararg, varkwarg) \
    YogVal varkwarg = YUNDEF; \
    YogVal vararg = YUNDEF; \
    YogVal blockarg = YUNDEF; \
    YogVal kwargs[2 * (kwargc)]; \
    YogVal args[(argc)]; \
    do { \
        unsigned int i = 0; \
        for (i = 0; i < 2 * (kwargc); i++) { \
            kwargs[i] = YUNDEF; \
        } \
        for (i = 0; i < (argc); i++) { \
            args[i] = YUNDEF; \
        } \
    } while (0); \
    PUSH_LOCALS3(ENV, varkwarg, vararg, blockarg); \
    PUSH_LOCALSX(ENV, 2 * (kwargc), kwargs); \
    PUSH_LOCALSX(ENV, (argc), args); \
\
    if (varkwargc == 1) { \
        varkwarg = POP(); \
    } \
\
    if (varargc == 1) { \
        vararg = POP(); \
    } \
\
    if (blockargc == 1) { \
        blockarg = POP(); \
    } \
\
    do { \
        unsigned int i; \
        for (i = kwargc; 0 < i; i--) { \
            kwargs[2 * i - 1] = POP(); \
            kwargs[2 * i - 2] = POP(); \
        } \
\
        for (i = argc; 0 < i; i--) { \
            args[i - 1] = POP(); \
        } \
    } while (0)
#define PUSH(val)   YogScriptFrame_push_stack(env, SCRIPT_FRAME(CUR_FRAME), val)
        OpCode op = PTR_AS(YogByteArray, CODE->insts)->items[PC];

#if 0
        do {
            printf("%p: ---------------- dump of stack ----------------\n", env);
            YogVal stack = SCRIPT_FRAME(CUR_FRAME)->stack;
            unsigned int stack_size = SCRIPT_FRAME(CUR_FRAME)->stack_size;
            if (0 < stack_size) {
                unsigned int i;
                for (i = stack_size; 0 < i; i--) {
                    YogVal_print(env, PTR_AS(YogValArray, stack)->items[i - 1]);
                }
            }
            else {
                printf("%p: stack is empty.\n", env);
            }

            DPRINTF("%p: PC=%u", env, PC);
            const char* opname = YogCode_get_op_name(op);
            DPRINTF("%p: op=%s", env, opname);
            printf("%p: ---------------- end of stack ----------------\n", env);
            fflush(stdout);
        } while (0);
#endif

        PC += sizeof(uint8_t);
        switch (op) {
#include "src/eval.inc"
        default:
            YOG_ASSERT(env, FALSE, "Unknown instruction.");
            break;
        }
#undef PUSH
#undef POP_ARGS
#undef JUMP
#undef THREAD
#undef CONSTS
#undef POP

#undef VM
#undef ENV
    }

    POP_BUF();
#undef CODE
#undef PC
#undef POP_BUF

    POP_FRAME();

    RETURN(env, YUNDEF);
}

YogVal 
YogEval_call_method_id(YogEnv* env, YogVal receiver, ID method, unsigned int argc, YogVal* args) 
{
    SAVE_ARG(env, receiver);
    YogVal attr = YUNDEF;
    YogVal retval = YUNDEF;
    PUSH_LOCALS2(env, attr, retval);

    attr = YogVal_get_attr(env, receiver, method);
    YOG_ASSERT(env, IS_PTR(attr), "attribute isn't object (0x%08x)", attr);
    retval = YogCallable_call(env, attr, argc, args);

    RETURN(env, retval);
}

YogVal 
YogEval_call_method_id2(YogEnv* env, YogVal receiver, ID method, unsigned int argc, YogVal* args, YogVal blockarg)
{
    SAVE_ARGS2(env, receiver, blockarg);
    YogVal attr = YUNDEF;
    YogVal retval = YUNDEF;
    PUSH_LOCALS2(env, attr, retval);

    attr = YogVal_get_attr(env, receiver, method);
    YOG_ASSERT(env, IS_PTR(attr), "Attribute isn't object.");
    retval = YogCallable_call2(env, attr, argc, args, blockarg);

    RETURN(env, retval);
}

void
YogEval_eval_package(YogEnv* env, YogVal pkg) 
{
    SAVE_ARG(env, pkg);

    YogVal frame = YUNDEF;
    YogVal code = YUNDEF;
    YogVal attrs = YUNDEF;
    PUSH_LOCALS3(env, frame, code, attrs);

    YogEval_push_finish_frame(env);

    frame = YogPackageFrame_new(env);
    code = PTR_AS(YogPackage, pkg)->code;
    setup_script_frame(env, frame, code);
    MODIFY(env, PTR_AS(YogNameFrame, frame)->self, pkg);
    attrs = PTR_AS(YogObj, pkg)->attrs;
    MODIFY(env, PTR_AS(YogNameFrame, frame)->vars, attrs);
    MODIFY(env, SCRIPT_FRAME(frame)->globals, PTR_AS(YogNameFrame, frame)->vars);
    PUSH_FRAME(frame);

    YogEval_mainloop(env);

    RETURN_VOID(env);
}

YogVal
YogEval_eval_file(YogEnv* env, FILE* fp, const char* filename, const char* pkg_name)
{
    YOG_ASSERT(env, fp != NULL, "file pointer is NULL");

    SAVE_LOCALS(env);

    YogVal stmts = YUNDEF;
    YogVal code = YUNDEF;
    YogVal pkg = YUNDEF;
    PUSH_LOCALS3(env, stmts, code, pkg);

    stmts = YogParser_parse_file(env, fp, FALSE);
    if (!IS_PTR(stmts)) {
        RETURN(env, YNIL);
    }
    code = YogCompiler_compile_module(env, filename, stmts);

    pkg = YogPackage_new(env);
    PTR_AS(YogPackage, pkg)->code = code;
    YogVM_register_package(env, env->vm, pkg_name, pkg);
    YogEval_eval_package(env, pkg);

    RETURN(env, pkg);
}

void
YogEval_push_finish_frame(YogEnv* env)
{
    SAVE_LOCALS(env);
    YogVal frame = YUNDEF;
    PUSH_LOCAL(env, frame);

    frame = YogFinishFrame_new(env);
    setup_script_frame(env, frame, env->vm->finish_code);

    PUSH_FRAME(frame);

    RETURN_VOID(env);
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
