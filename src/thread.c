#include <setjmp.h>
#include <stdio.h>
#include <string.h>
#include "yog/arg.h"
#include "yog/binary.h"
#include "yog/block.h"
#include "yog/code.h"
#include "yog/error.h"
#include "yog/function.h"
#include "yog/method.h"
#include "yog/opcodes.h"
#include "yog/st.h"
#include "yog/yog.h"

#define CUR_FRAME   (ENV_TH(env)->cur_frame)

#define PUSH_FRAME(f)   do { \
    PTR_AS(YogFrame, (f))->prev = CUR_FRAME; \
    CUR_FRAME = (f); \
} while (0)

#define POP_FRAME()     do { \
    CUR_FRAME = PTR_AS(YogFrame, CUR_FRAME)->prev; \
} while (0)

YogVal 
YogThread_call_method(YogEnv* env, YogThread* th, YogVal receiver, const char* method, unsigned int argc, YogVal* args) 
{
    SAVE_ARG(env, receiver);
#if 0
    PUSH_LOCALSX(env, argc, args);
#endif

    ID id = YogVm_intern(env, ENV_VM(env), method);
    YogVal retval = YogThread_call_method_id(env, th, receiver, id, argc, args);

    RETURN(env, retval);
}

static void 
fill_args(YogEnv* env, YogVal arg_info, uint8_t posargc, YogVal posargs[], YogVal blockarg, uint8_t kwargc, YogVal kwargs[], YogVal vararg, YogVal varkwarg, unsigned int argc, YogVal args, unsigned int args_offset) 
{
    SAVE_ARGS5(env, arg_info, blockarg, vararg, varkwarg, args);
#if 0
    PUSH_LOCALSX(env, posargc, posargs);
    PUSH_LOCALSX(env, 2 * kwargc, kwargs);
#endif

    unsigned int i = 0;
    unsigned int arg_argc = ARG_INFO(arg_info)->argc;
    unsigned int arg_blockargc = ARG_INFO(arg_info)->blockargc;
    unsigned int size = arg_argc + arg_blockargc;
    for (i = 0; i < size; i++) {
        PTR_AS(YogValArray, args)->items[args_offset + i] = YUNDEF;
    }

    if (ARG_INFO(arg_info)->argc < posargc) {
        for (i = 0; i < ARG_INFO(arg_info)->argc; i++) {
            PTR_AS(YogValArray, args)->items[args_offset + i] = posargs[i];
        }
        YOG_ASSERT(env, ARG_INFO(arg_info)->varargc == 1, "Too many arguments.");
        unsigned int argc = ARG_INFO(arg_info)->argc;
        unsigned int blockargc = ARG_INFO(arg_info)->blockargc;
        unsigned int index = argc + blockargc;
        YogVal array = PTR_AS(YogValArray, args)->items[args_offset + index];
        for (i = ARG_INFO(arg_info)->argc; i < posargc; i++) {
            YogArray_push(env, array, posargs[i]);
        }
    }
    else {
        for (i = 0; i < posargc; i++) {
            PTR_AS(YogValArray, args)->items[args_offset + i] = posargs[i];
        }
    }

    if (!IS_UNDEF(blockarg)) {
        YOG_ASSERT(env, ARG_INFO(arg_info)->blockargc == 1, "Can't accept block argument.");
        unsigned int index = ARG_INFO(arg_info)->argc;
        PTR_AS(YogValArray, args)->items[args_offset + index] = blockarg;
    }

    for (i = 0; i < kwargc; i++) {
        YogVal name = kwargs[2 * i];
        ID id = VAL2ID(name);
        unsigned int j = 0;
        for (j = 0; j < ARG_INFO(arg_info)->argc; j++) {
            ID argname = ARG_INFO(arg_info)->argnames[j];
            if (argname == id) {
                YOG_ASSERT(env, !IS_UNDEF(PTR_AS(YogValArray, args)->items[j]), "Argument specified twice.");
                YogVal val = kwargs[2 * i + 1];
                PTR_AS(YogValArray, args)->items[args_offset + j] = val;
                break;
            }
        }
        if (j == ARG_INFO(arg_info)->argc) {
            ID argname = ARG_INFO(arg_info)->blockargname;
            if (argname == id) {
                YOG_ASSERT(env, !IS_UNDEF(PTR_AS(YogValArray, args)->items[args_offset + j]), "Argument specified twice.");
                PTR_AS(YogValArray, args)->items[args_offset + argc - 1] = blockarg;
            }
        }
    }

    RETURN_VOID(env);
}

static void 
fill_builtin_function_args(YogEnv* env, YogVal f, uint8_t posargc, YogVal posargs[], YogVal blockarg, uint8_t kwargc, YogVal kwargs[], YogVal vararg, YogVal varkwarg, YogVal args)
{
    SAVE_ARGS5(env, f, blockarg, vararg, varkwarg, args);
#if 0
    PUSH_LOCALSX(env, posargc, posargs);
    PUSH_LOCALSX(env, 2 * kwargc, kwargs);
#endif

    unsigned int argc = YogValArray_size(env, PTR_AS(YogValArray, args));

    fill_args(env, OBJ_AS(YogBuiltinFunction, f)->arg_info, posargc, posargs, blockarg, kwargc, kwargs, vararg, varkwarg, argc, args, 0);

    int required_argc = 0;
    if (OBJ_AS(YogBuiltinFunction, f)->required_argc < 0) {
        required_argc = argc;
    }
    else {
        required_argc = OBJ_AS(YogBuiltinFunction, f)->required_argc;
    }
    unsigned int i = 0;
    for (i = 0; i < required_argc; i++) {
        YogVal val = YogValArray_at(env, PTR_AS(YogValArray, args), i);
        YOG_ASSERT(env, !IS_UNDEF(val), "Argument not specified.");
    }
    for (i = required_argc; i < argc; i++) {
        YogVal val = YogValArray_at(env, PTR_AS(YogValArray, args), i);
        if (IS_UNDEF(val)) {
            PTR_AS(YogValArray, args)->items[i] = YNIL;
        }
    }

    RETURN_VOID(env);
}

static YogVal 
call_builtin_function(YogEnv* env, YogThread* th, YogVal f, YogVal self, YogVal args) 
{
    SAVE_ARGS3(env, f, self, args);

    YogCFrame* frame = YogCFrame_new(env);
    frame->self = self;
    frame->args = PTR_AS(YogValArray, args);
    frame->f = PTR_AS(YogBuiltinFunction, f);
    PUSH_FRAME(PTR2VAL(frame));

    YogVal retval = (*PTR_AS(YogBuiltinFunction, f)->f)(env);

    POP_FRAME();

    RETURN(env, retval);
}

#define DECL_ARGS \
    YogVal f = method->f; \
    YogArgInfo* arg_info = &OBJ_AS(YogBuiltinFunction, f)->arg_info; \
    YOG_ASSERT(env, (posargc <= arg_info->argc) || (0 < arg_info->varargc), "Too many argument(s)."); \
    unsigned int argc = arg_info->argc + arg_info->blockargc + arg_info->varargc + arg_info->kwargc; \
    YogValArray* args = YogValArray_new(env, argc); \
    if (0 < arg_info->varargc) { \
        YogVal vararg = YogArray_new(env); \
        unsigned int index = arg_info->argc + arg_info->blockargc; \
        args->items[index] = vararg; \
    }

static YogVal 
call_builtin_unbound_method(YogEnv* env, YogThread* th, YogVal receiver, YogVal method, uint8_t posargc, YogVal posargs[], YogVal blockarg, uint8_t kwargc, YogVal kwargs[], YogVal vararg, YogVal varkwarg) 
{
    SAVE_ARGS5(env, receiver, method, blockarg, vararg, varkwarg);
#if 0
    PUSH_LOCALSX(env, posargc, posargs);
    PUSH_LOCALSX(env, 2 * kwargc, kwargs);
#endif

    YogVal args = YUNDEF;
    YogVal f = YUNDEF;
    PUSH_LOCALS2(env, args, f);

#if 0
    DECL_ARGS;
#endif
    f = OBJ_AS(YogBuiltinUnboundMethod, method)->f;
    YogVal arg_info = PTR_AS(YogBuiltinFunction, f)->arg_info;
    unsigned int f_argc = PTR_AS(YogArgInfo, arg_info)->argc;
    unsigned int f_blockargc = PTR_AS(YogArgInfo, arg_info)->blockargc;
    unsigned int f_varargc = PTR_AS(YogArgInfo, arg_info)->varargc;
    unsigned int f_kwargc = PTR_AS(YogArgInfo, arg_info)->kwargc;
    YOG_ASSERT(env, (posargc <= f_argc) || (0 < f_varargc), "Too many argument(s).");
    unsigned int argc = f_argc + f_blockargc + f_varargc + f_kwargc;
    args = PTR2VAL(YogValArray_new(env, argc));
    if (0 < f_varargc) {
        YogVal vararg = YogArray_new(env);
        unsigned int index = f_argc + f_blockargc;
        PTR_AS(YogValArray, args)->items[index] = vararg;
    }

    fill_builtin_function_args(env, f, posargc, posargs, blockarg, kwargc, kwargs, vararg, varkwarg, args);

    YogVal retval = call_builtin_function(env, th, f, receiver, args);

    RETURN(env, retval);
}

static YogVal 
call_builtin_bound_method(YogEnv* env, YogThread* th, YogVal method, uint8_t posargc, YogVal posargs[], YogVal blockarg, uint8_t kwargc, YogVal kwargs[], YogVal vararg, YogVal varkwarg) 
{
    SAVE_ARGS4(env, method, blockarg, vararg, varkwarg);
#if 0
    PUSH_LOCALSX(env, posargc, posargs);
    PUSH_LOCALSX(env, 2 * kwargc, kwargs);
#endif

    YogVal args = YUNDEF;
    YogVal f = YUNDEF;
    PUSH_LOCALS2(env, args, f);

#if 0
    DECL_ARGS;
#endif
    f = OBJ_AS(YogBuiltinBoundMethod, method)->f;
    YogVal arg_info = OBJ_AS(YogBuiltinFunction, f)->arg_info;
    unsigned int f_argc = PTR_AS(YogArgInfo, arg_info)->argc;
    unsigned int f_blockargc = PTR_AS(YogArgInfo, arg_info)->blockargc;
    unsigned int f_varargc = PTR_AS(YogArgInfo, arg_info)->varargc;
    unsigned int f_kwargc = PTR_AS(YogArgInfo, arg_info)->kwargc;
    YOG_ASSERT(env, (posargc <= f_argc) || (0 < f_varargc), "Too many argument(s).");
    unsigned int argc = f_argc + f_blockargc + f_varargc + f_kwargc;
    args = PTR2VAL(YogValArray_new(env, argc));
    if (0 < f_varargc) {
        YogVal vararg = YogArray_new(env);
        unsigned int index = f_argc + f_blockargc;
        PTR_AS(YogValArray, args)->items[index] = vararg;
    }

    fill_builtin_function_args(env, f, posargc, posargs, blockarg, kwargc, kwargs, vararg, varkwarg, args);

    YogVal self = OBJ_AS(YogBuiltinBoundMethod, method)->self;
    YogVal retval = call_builtin_function(env, th, f, self, args);

    RETURN(env, retval);
}

#undef DECL_ARGS

static void 
setup_script_method(YogEnv* env, YogVal method, YogVal code) 
{
    SAVE_ARGS2(env, method, code);

    SCRIPT_METHOD(method)->code = code;
    SCRIPT_METHOD(method)->globals = SCRIPT_FRAME(CUR_FRAME)->globals;

    unsigned int outer_size = PTR_AS(YogCode, code)->outer_size;
    YogOuterVars* outer_vars = YogOuterVars_new(env, outer_size);

    YogVal frame = PTR_AS(YogFrame, CUR_FRAME)->prev;
    unsigned int i = 0;
    for (i = 0; i < outer_size; i++) {
        YOG_ASSERT(env, IS_PTR(frame), "frame is not object");
        YOG_ASSERT(env, PTR_AS(YogFrame, frame)->type == FRAME_METHOD, "frame type isn't FRAME_METHOD");

        outer_vars->items[i] = LOCAL_VARS(frame);

        frame = PTR_AS(YogFrame, frame)->prev;
    }

    SCRIPT_METHOD(method)->outer_vars = outer_vars;

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
    YogValArray* stack = YogValArray_new(env, stack_size);

    SCRIPT_FRAME(frame)->pc = 0;
    SCRIPT_FRAME(frame)->code = code;
    SCRIPT_FRAME(frame)->stack = stack;

    RETURN_VOID(env);
}

static void 
call_code(YogEnv* env, YogThread* th, YogVal self, YogVal code, uint8_t posargc, YogVal posargs[], YogVal blockarg, uint8_t kwargc, YogVal kwargs[], YogVal vararg, YogVal varkwarg)
{
    SAVE_ARGS5(env, self, code, blockarg, vararg, varkwarg);
#if 0
    PUSH_LOCALSX(env, posargc, posargs);
    PUSH_LOCALSX(env, 2 * kwargc, kwargs);
#endif

    YogVal vars = YUNDEF;
    YogVal frame = YUNDEF;
    PUSH_LOCALS2(env, vars, frame);

    unsigned int local_vars_count = PTR_AS(YogCode, code)->local_vars_count;
    vars = PTR2VAL(YogValArray_new(env, local_vars_count));
    PTR_AS(YogValArray, vars)->items[0] = self;

    YogVal arg_info = PTR_AS(YogCode, code)->arg_info;
    unsigned int code_argc = PTR_AS(YogArgInfo, arg_info)->argc;
    unsigned int code_blockargc = PTR_AS(YogArgInfo, arg_info)->blockargc;
    unsigned int code_varargc = PTR_AS(YogArgInfo, arg_info)->varargc;
    unsigned int code_kwargc = PTR_AS(YogArgInfo, arg_info)->kwargc;
    unsigned int argc = code_argc + code_blockargc + code_varargc + code_kwargc;
    fill_args(env, arg_info, posargc, posargs, blockarg, kwargc, kwargs, vararg, varkwarg, argc, vars, 1);

    frame = PTR2VAL(YogMethodFrame_new(env));
    setup_script_frame(env, frame, code);
    PTR_AS(YogMethodFrame, frame)->vars = PTR_AS(YogValArray, vars);
    PTR_AS(YogScriptFrame, frame)->globals = SCRIPT_FRAME(CUR_FRAME)->globals;

    PUSH_FRAME(frame);

    RETURN_VOID(env);
}

#define PUSH(val)   YogScriptFrame_push_stack(env, SCRIPT_FRAME(CUR_FRAME), val)

static void 
call_method(YogEnv* env, YogThread* th, YogVal unbound_self, YogVal callee, uint8_t posargc, YogVal posargs[], YogVal blockarg, uint8_t kwargc, YogVal kwargs[], YogVal vararg, YogVal varkwarg)
{
    SAVE_ARGS5(env, unbound_self, callee, blockarg, vararg, varkwarg);
#if 0
    PUSH_LOCALSX(env, posargc, posargs);
    PUSH_LOCALSX(env, 2 * kwargc, kwargs);
#endif

    YOG_ASSERT(env, IS_OBJ(callee), "Callee is not object.");
    if (IS_OBJ_OF(cBuiltinBoundMethod, callee)) {
        YogVal val = call_builtin_bound_method(env, th, callee, posargc, posargs, blockarg, kwargc, kwargs, vararg, varkwarg);
        PUSH(val);
    }
    else if (IS_OBJ_OF(cBoundMethod, callee)) {
        YogVal self = PTR_AS(YogBoundMethod, callee)->self;
        YogVal code = SCRIPT_METHOD(callee)->code;
        call_code(env, th, self, code, posargc, posargs, blockarg, kwargc, kwargs, vararg, varkwarg);
    }
    else if (IS_OBJ_OF(cBuiltinUnboundMethod, callee)) {
        YogVal self = unbound_self;
        YogVal val = call_builtin_unbound_method(env, th, self, callee, posargc, posargs, blockarg, kwargc, kwargs, vararg, varkwarg);
        PUSH(val);
    }
    else if (IS_OBJ_OF(cUnboundMethod, callee)) {
        YogVal self = unbound_self;
        YogVal code = SCRIPT_METHOD(callee)->code;
        call_code(env, th, self, code, posargc, posargs, blockarg, kwargc, kwargs, vararg, varkwarg);
    }
    else {
        YOG_ASSERT(env, FALSE, "Callee is not callable.");
    }

    RETURN_VOID(env);
}

static BOOL
lookup_frame_vars(YogEnv* env, YogVal frame, ID name, YogVal* val) 
{
    switch (PTR_AS(YogFrame, frame)->type) {
    case FRAME_C:
        break;
    case FRAME_METHOD:
        {
            YogVal code = SCRIPT_FRAME(frame)->code;
            unsigned int count = PTR_AS(YogCode, code)->local_vars_count;
            ID* names = PTR_AS(YogCode, code)->local_vars_names;
            unsigned int i = 0;
            for (i = 0; i < count; i++) {
                if (names[i] == name) {
                    *val = METHOD_FRAME(frame)->vars->items[i];
                    return TRUE;
                }
            }
        }
        break;
    case FRAME_NAME:
        {
            YogTable* vars = NAME_FRAME(frame)->vars;
            if (YogTable_lookup(env, PTR2VAL(vars), ID2VAL(name), val)) {
                return TRUE;
            }
        }
        break;
    default:
        YOG_BUG(env, "unknown frame type (0x%x)", PTR_AS(YogFrame, frame)->type);
        break;
    }

    return FALSE;
}

static YogVal 
lookup_builtins(YogEnv* env, ID name) 
{
    YogVal builtins_name = ID2VAL(INTERN(BUILTINS));
    YogVal builtins = YUNDEF;
    YogVm* vm = ENV_VM(env);
    if (!YogTable_lookup(env, vm->pkgs, builtins_name, &builtins)) {
        YOG_ASSERT(env, FALSE, "Can't find builtins package.");
    }

    YogPackage* pkg = OBJ_AS(YogPackage, builtins);
    YogVal key = ID2VAL(name);
    YogVal val = YUNDEF;
    if (!YogTable_lookup(env, YOGOBJ(pkg)->attrs, key, &val)) {
        YOG_ASSERT(env, FALSE, "Can't find builtins attribute.");
    }

    return val;
}

static YogVal
mainloop(YogEnv* env, YogThread* th, YogVal frame, YogVal code) 
{
    SAVE_ARGS2(env, frame, code);

#if 0
#   define DUMP_CODE    YogCode_dump(env, code)
#else
#   define DUMP_CODE
#endif
    DUMP_CODE;

    PUSH_FRAME(frame);

#define POP_BUF()   ENV_TH(env)->jmp_buf_list = ENV_TH(env)->jmp_buf_list->prev
#define PC          (SCRIPT_FRAME(CUR_FRAME)->pc)
#undef CODE
#define CODE        PTR_AS(YogCode, SCRIPT_FRAME(CUR_FRAME)->code)
    YogJmpBuf jmpbuf;
    int status = 0;
    if ((status = setjmp(jmpbuf.buf)) == 0) {
        jmpbuf.prev = ENV_TH(env)->jmp_buf_list;
        ENV_TH(env)->jmp_buf_list = &jmpbuf;
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
            YogJmpBuf* list = ENV_TH(env)->jmp_buf_list;
            if (list != NULL) {
                longjmp(list->buf, status);
            }

#define PRINT(...)  fprintf(stderr, __VA_ARGS__)
            PRINT("Traceback (most recent call last):\n");

            YogException* exc = OBJ_AS(YogException, ENV_TH(env)->jmp_val);
            YogVal st = exc->stack_trace;
#define ID2NAME(id)     YogVm_id2name(env, ENV_VM(env), id)
            while (IS_PTR(st)) {
                PRINT("  File ");
                const char* filename = PTR_AS(YogStackTraceEntry, st)->filename;
                if (filename != NULL) {
                    PRINT("\"%s\"", filename);
                }
                else {
                    PRINT("builtin");
                }

                unsigned int lineno = PTR_AS(YogStackTraceEntry, st)->lineno;
                if (0 < lineno) {
                    PRINT(", line %d", lineno);
                }

                PRINT(", in ");
                ID klass_name = PTR_AS(YogStackTraceEntry, st)->klass_name;
                ID func_name = PTR_AS(YogStackTraceEntry, st)->func_name;
                if (klass_name != INVALID_ID) {
                    if (func_name != INVALID_ID) {
                        const char* s = ID2NAME(klass_name);
                        const char* t = ID2NAME(func_name);
                        PRINT("%s#%s", s, t);
                    }
                    else {
                        const char* name = ID2NAME(klass_name);
                        PRINT("<class %s>", name);
                    }
                }
                else {
                    const char* name = ID2NAME(func_name);
                    PRINT("%s", name);
                }
                PRINT("\n");

                st = PTR_AS(YogStackTraceEntry, st)->lower;
            }

            YogVal klass = YOGBASICOBJ(exc)->klass;
            const char* name = ID2NAME(OBJ_AS(YogKlass, klass)->name);
            /* dirty hack */
            size_t len = strlen(name);
            char s[len + 1];
            strcpy(s, name);
#undef ID2NAME
            YogVal val = YogThread_call_method(env, ENV_TH(env), exc->message, "to_s", 0, NULL);
            YogString* msg = OBJ_AS(YogString, val);
            PRINT("%s: %s\n", s, msg->body->items);
#undef PRINT

            RETURN(env, INT2VAL(-1));
        }
    }

    while (PC < CODE->insts->size) {
#define ENV             (env)
#define VM              (ENV_VM(ENV))
        if (VM->need_gc || VM->gc_stress) {
            YogVm_gc(ENV, VM);
            th = VM->thread;
            ENV_TH(env) = th;
            VM->need_gc = FALSE;
        }

#define POP()           (YogScriptFrame_pop_stack(env, SCRIPT_FRAME(CUR_FRAME)))
#define CONSTS(index)   (YogValArray_at(env, CODE->consts, index))
#define THREAD          (ENV_TH(env))
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
        OpCode op = CODE->insts->items[PC];

#if 0
        do {
            printf("------------------------------ dump of stack ------------------------------\n");
            YogValArray* stack = SCRIPT_FRAME(CUR_FRAME)->stack;
            printf("%s:%d stack=%p\n", __FILE__, __LINE__, stack);
            unsigned int stack_size = SCRIPT_FRAME(CUR_FRAME)->stack_size;
            if (0 < stack_size) {
                unsigned int i;
                for (i = stack_size; 0 < i; i--) {
                    YogVal_print(env, stack->items[i - 1]);
                }
            }
            else {
                printf("stack is empty.\n");
            }

            printf("%s:%d PC=%d\n", __FILE__, __LINE__, PC);
            const char* opname = YogCode_get_op_name(op);
            printf("%s:%d op=%s\n", __FILE__, __LINE__, opname);
            printf("------------------------------ end of stack ------------------------------\n");
            fflush(stdout);
        } while (0);
#endif

        PC += sizeof(uint8_t);
        switch (op) {
#include "src/thread.inc"
        default:
            YOG_ASSERT(env, FALSE, "Unknown instruction.");
            break;
        }
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

static YogVal 
eval_code(YogEnv* env, YogThread* th, YogVal code, YogVal receiver, unsigned int argc, YogVal args[]) 
{
    SAVE_ARGS2(env, code, receiver);
#if 0
    PUSH_LOCALSX(env, argc, args);
#endif

    YogVal undef = YUNDEF;
    call_code(env, th, receiver, code, argc, args, undef, 0, NULL, undef, undef);

    YogVal retval = mainloop(env, th, CUR_FRAME, code);

    RETURN(env, retval);
}

YogVal 
YogThread_call_block(YogEnv* env, YogThread* th, YogVal block, unsigned int argc, YogVal* args) 
{
    SAVE_ARG(env, block);
#if 0
    PUSH_LOCALSX(env, argc, args);
#endif

    YogVal retval = YUNDEF;
    if (IS_OBJ_OF(cPackageBlock, block)) {
        YogVal code = YUNDEF;
        YogVal vars = YUNDEF;
        YogVal frame = YUNDEF;
        YogVal arg_info = YUNDEF;
        PUSH_LOCALS4(env, code, vars, frame, arg_info);

        code = PTR_AS(YogBasicBlock, block)->code;
        arg_info = PTR_AS(YogCode, code)->arg_info;

#define SET_VAR(name) do { \
    YogVal symbol = ID2VAL(name); \
    YogTable_insert(env, vars, symbol, args[i]); \
} while (0)
        vars = PTR2VAL(PTR_AS(YogPackageBlock, block)->vars);
        unsigned int argc = PTR_AS(YogArgInfo, arg_info)->argc;
        unsigned int i = 0;
        for (i = 0; i < argc; i++) {
            ID name = PTR_AS(YogArgInfo, arg_info)->argnames[i];
            SET_VAR(name);
        }
        unsigned int blockargc = PTR_AS(YogArgInfo, arg_info)->blockargc;
        if (i < argc + blockargc) {
            ID name = PTR_AS(YogArgInfo, arg_info)->blockargname;
            SET_VAR(name);
            i++;
        }
#undef SET_VAR

        frame = YogPackageFrame_new(env);
        setup_script_frame(env, frame, code);
        YogVal self = PTR_AS(YogPackageBlock, block)->self;
        PTR_AS(YogNameFrame, frame)->self = self;
        PTR_AS(YogNameFrame, frame)->vars = PTR_AS(YogTable, vars);

        YogTable* globals = NULL;
        YogVal f = PTR_AS(YogFrame, CUR_FRAME)->prev;
        while (IS_PTR(f)) {
            YogFrameType type = PTR_AS(YogFrame, f)->type;
            if ((type == FRAME_METHOD) || (type == FRAME_NAME)) {
                globals = SCRIPT_FRAME(f)->globals;
                break;
            }

            f = PTR_AS(YogFrame, f)->prev;
        }
        YOG_ASSERT(env, globals != NULL, "globals is NULL");
        SCRIPT_FRAME(frame)->globals = globals;

        retval = mainloop(env, th, frame, code);

        POP_LOCALS(env);
    }
    else {
        YogVal frame = YUNDEF;
        YogVal code = YUNDEF;
        PUSH_LOCALS2(env, frame, code);

        frame = PTR2VAL(YogMethodFrame_new(env));
        code = PTR_AS(YogBasicBlock, block)->code;
        setup_script_frame(env, frame, code);
        SCRIPT_FRAME(frame)->globals = PTR_AS(YogBlock, block)->globals;
        SCRIPT_FRAME(frame)->outer_vars = PTR_AS(YogBlock, block)->outer_vars;
        PTR_AS(YogMethodFrame, frame)->vars = PTR_AS(YogBlock, block)->locals;

        fill_args(env, PTR_AS(YogCode, code)->arg_info, argc, args, YUNDEF, 0, NULL, YUNDEF, YUNDEF, PTR_AS(YogMethodFrame, frame)->vars->size, PTR2VAL(PTR_AS(YogMethodFrame, frame)->vars), 1);

        retval = mainloop(env, th, frame, code);

        POP_LOCALS(env);
    }

    RETURN(env, retval);
}

YogVal 
YogThread_call_method_id(YogEnv* env, YogThread* th, YogVal receiver, ID method, unsigned int argc, YogVal* args) 
{
    SAVE_ARG(env, receiver);
#if 0
    PUSH_LOCALSX(env, argc, args);
#endif

    YogVal attr = YogVal_get_attr(env, receiver, method);
    YOG_ASSERT(env, IS_OBJ(attr), "Attribute isn't object.");

    YogVal retval = YUNDEF;
    YogVal undef = YUNDEF;
    if (IS_OBJ_OF(cBuiltinBoundMethod, attr)) {
        retval = call_builtin_bound_method(env, th, attr, argc, args, undef, 0, NULL, undef, undef);
    }
    else if (IS_OBJ_OF(cBoundMethod, attr)) {
        YogBoundMethod* method = OBJ_AS(YogBoundMethod, attr);
        YogVal self = method->self;
        YogVal code = ((YogScriptMethod*)method)->code;
        retval = eval_code(env, th, code, self, argc, args);
    }
    else if (IS_OBJ_OF(cBuiltinUnboundMethod, attr)) {
        retval = call_builtin_unbound_method(env, th, receiver, attr, argc, args, undef, 0, NULL, undef, undef);
    }
    else if (IS_OBJ_OF(cUnboundMethod, attr)) {
        YogUnboundMethod* method = OBJ_AS(YogUnboundMethod, attr);
        YogVal code = ((YogScriptMethod*)method)->code;
        retval = eval_code(env, th, code, receiver, argc, args);
    }
    else {
        YOG_ASSERT(env, FALSE, "Callee is not callable.");
    }

    RETURN(env, retval);
}

void 
YogThread_eval_package(YogEnv* env, YogThread* th, YogVal pkg) 
{
    SAVE_ARG(env, pkg);

    YogVal frame = YUNDEF;
    YogVal code = YUNDEF;
    YogVal attrs = YUNDEF;
    PUSH_LOCALS3(env, frame, code, attrs);

    frame = YogPackageFrame_new(env);
    code = PTR2VAL(PTR_AS(YogPackage, pkg)->code);
    setup_script_frame(env, frame, code);
    PTR_AS(YogNameFrame, frame)->self = pkg;
    attrs = PTR_AS(YogObj, pkg)->attrs;
    PTR_AS(YogNameFrame, frame)->vars = PTR_AS(YogTable, attrs);
    SCRIPT_FRAME(frame)->globals = PTR_AS(YogNameFrame, frame)->vars;

    mainloop(env, th, frame, code);

    RETURN_VOID(env);
}

void 
YogThread_keep_children(YogEnv* env, void* ptr, ObjectKeeper keeper)
{
    YogThread* th = ptr;

    th->cur_frame = YogVal_keep(env, th->cur_frame, keeper);
    th->jmp_val = YogVal_keep(env, th->jmp_val, keeper);

    YogLocals* locals = th->locals;
    while (locals != NULL) {
        unsigned int i;
        for (i = 0; i < locals->num_vals; i++) {
            YogVal* vals = locals->vals[i];
            if (vals == NULL) {
                continue;
            }

            unsigned int j;
            for (j = 0; j < locals->size; j++) {
                YogVal* val = &vals[j];
                *val = YogVal_keep(env, *val, keeper);
            }
        }

        locals = locals->next;
    }
}

void 
YogThread_initialize(YogEnv* env, YogThread* thread) 
{
    thread->cur_frame = YNIL;
    thread->jmp_buf_list = NULL;
    thread->jmp_val = YUNDEF;
    thread->locals = NULL;
}

YogThread*
YogThread_new(YogEnv* env) 
{
    YogThread* th = ALLOC_OBJ(env, YogThread_keep_children, NULL, YogThread);
    YogThread_initialize(env, th);

    return th;
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
