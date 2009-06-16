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
#include "yog/method.h"
#include "yog/package.h"
#include "yog/parser.h"
#include "yog/string.h"
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
#if 0
    PUSH_LOCALSX(env, argc, args);
#endif

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

static void 
fill_args(YogEnv* env, YogVal arg_info, uint8_t posargc, YogVal posargs[], YogVal blockarg, uint8_t kwargc, YogVal kwargs[], YogVal vararg, YogVal varkwarg, unsigned int argc, YogVal args, unsigned int args_offset) 
{
    if (!IS_PTR(arg_info)) {
        return;
    }

    SAVE_ARGS5(env, arg_info, blockarg, vararg, varkwarg, args);

    YogVal array = YUNDEF;
    PUSH_LOCAL(env, array);
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
            YogVal* items = PTR_AS(YogValArray, args)->items;
            YogVal arg = posargs[i];
            MODIFY(env, items[args_offset + i], arg);
        }
        YOG_ASSERT(env, ARG_INFO(arg_info)->varargc == 1, "Too many arguments.");
        unsigned int argc = ARG_INFO(arg_info)->argc;
        unsigned int blockargc = ARG_INFO(arg_info)->blockargc;
        unsigned int index = argc + blockargc;
        array = PTR_AS(YogValArray, args)->items[args_offset + index];
        for (i = ARG_INFO(arg_info)->argc; i < posargc; i++) {
            YogArray_push(env, array, posargs[i]);
        }
    }
    else {
        for (i = 0; i < posargc; i++) {
            YogVal* items = PTR_AS(YogValArray, args)->items;
            YogVal arg = posargs[i];
            MODIFY(env, items[args_offset + i], arg);
        }
    }

    if (!IS_UNDEF(blockarg)) {
        YOG_ASSERT(env, ARG_INFO(arg_info)->blockargc == 1, "Can't accept block argument.");
        YogVal* items = PTR_AS(YogValArray, args)->items;
        unsigned int index = ARG_INFO(arg_info)->argc;
        MODIFY(env, items[args_offset + index], blockarg);
    }

    for (i = 0; i < kwargc; i++) {
        YogVal name = kwargs[2 * i];
        ID id = VAL2ID(name);
        unsigned int j = 0;
        for (j = 0; j < ARG_INFO(arg_info)->argc; j++) {
            YogVal argnames = ARG_INFO(arg_info)->argnames;
            ID argname = PTR_AS(ID, argnames)[j];
            if (argname == id) {
                YOG_ASSERT(env, !IS_UNDEF(PTR_AS(YogValArray, args)->items[j]), "Argument specified twice.");
                YogVal* items = PTR_AS(YogValArray, args)->items;
                YogVal val = kwargs[2 * i + 1];
                MODIFY(env, items[args_offset + j], val);
                break;
            }
        }
        if (j == ARG_INFO(arg_info)->argc) {
            ID argname = ARG_INFO(arg_info)->blockargname;
            if (argname == id) {
                YOG_ASSERT(env, !IS_UNDEF(PTR_AS(YogValArray, args)->items[args_offset + j]), "Argument specified twice.");
                YogVal* items = PTR_AS(YogValArray, args)->items;
                MODIFY(env, items[args_offset + argc - 1], blockarg);
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

    unsigned int argc = YogValArray_size(env, args);

    fill_args(env, PTR_AS(YogBuiltinFunction, f)->arg_info, posargc, posargs, blockarg, kwargc, kwargs, vararg, varkwarg, argc, args, 0);

    int required_argc = 0;
    if (PTR_AS(YogBuiltinFunction, f)->required_argc < 0) {
        required_argc = argc;
    }
    else {
        required_argc = PTR_AS(YogBuiltinFunction, f)->required_argc;
    }
    unsigned int i = 0;
    for (i = 0; i < required_argc; i++) {
        YogVal val = YogValArray_at(env, args, i);
        YOG_ASSERT(env, !IS_UNDEF(val), "Argument not specified.");
    }
    for (i = required_argc; i < argc; i++) {
        YogVal val = YogValArray_at(env, args, i);
        if (IS_UNDEF(val)) {
            PTR_AS(YogValArray, args)->items[i] = YNIL;
        }
    }

    RETURN_VOID(env);
}

static YogVal 
call_builtin_function(YogEnv* env, YogVal f, YogVal self, YogVal args) 
{
    SAVE_ARGS3(env, f, self, args);

    YogVal frame = YogCFrame_new(env);
    MODIFY(env, PTR_AS(YogCFrame, frame)->self, self);
    MODIFY(env, PTR_AS(YogCFrame, frame)->args, args);
    MODIFY(env, PTR_AS(YogCFrame, frame)->f, f);
    PUSH_FRAME(frame);

    YogVal retval = (*PTR_AS(YogBuiltinFunction, f)->f)(env);

    POP_FRAME();

    RETURN(env, retval);
}

#define DECL_ARGS \
    YogVal f = method->f; \
    YogArgInfo* arg_info = &PTR_AS(YogBuiltinFunction, f)->arg_info; \
    YOG_ASSERT(env, (posargc <= arg_info->argc) || (0 < arg_info->varargc), "Too many argument(s)."); \
    unsigned int argc = arg_info->argc + arg_info->blockargc + arg_info->varargc + arg_info->kwargc; \
    YogVal args = YogValArray_new(env, argc); \
    if (0 < arg_info->varargc) { \
        YogVal vararg = YogArray_new(env); \
        unsigned int index = arg_info->argc + arg_info->blockargc; \
        YogVal* items = PTR_AS(YogValArray, args)->items; \
        MODIFY(env, items[index], vararg); \
    }

static YogVal 
call_builtin_unbound_method(YogEnv* env, YogVal receiver, YogVal method, uint8_t posargc, YogVal posargs[], YogVal blockarg, uint8_t kwargc, YogVal kwargs[], YogVal vararg, YogVal varkwarg) 
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
    f = PTR_AS(YogBuiltinUnboundMethod, method)->f;
    YogVal arg_info = PTR_AS(YogBuiltinFunction, f)->arg_info;
    unsigned int f_argc = PTR_AS(YogArgInfo, arg_info)->argc;
    unsigned int f_blockargc = PTR_AS(YogArgInfo, arg_info)->blockargc;
    unsigned int f_varargc = PTR_AS(YogArgInfo, arg_info)->varargc;
    unsigned int f_kwargc = PTR_AS(YogArgInfo, arg_info)->kwargc;
    YOG_ASSERT(env, (posargc <= f_argc) || (0 < f_varargc), "Too many argument(s).");
    unsigned int argc = f_argc + f_blockargc + f_varargc + f_kwargc;
    args = YogValArray_new(env, argc);
    if (0 < f_varargc) {
        YogVal vararg = YogArray_new(env);
        unsigned int index = f_argc + f_blockargc;
        YogVal* items = PTR_AS(YogValArray, args)->items;
        MODIFY(env, items[index], vararg);
    }

    fill_builtin_function_args(env, f, posargc, posargs, blockarg, kwargc, kwargs, vararg, varkwarg, args);

    YogVal retval = call_builtin_function(env, f, receiver, args);

    RETURN(env, retval);
}

static YogVal 
call_builtin_bound_method(YogEnv* env, YogVal method, uint8_t posargc, YogVal posargs[], YogVal blockarg, uint8_t kwargc, YogVal kwargs[], YogVal vararg, YogVal varkwarg) 
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
    f = PTR_AS(YogBuiltinBoundMethod, method)->f;
    YogVal arg_info = PTR_AS(YogBuiltinFunction, f)->arg_info;
    unsigned int f_argc = PTR_AS(YogArgInfo, arg_info)->argc;
    unsigned int f_blockargc = PTR_AS(YogArgInfo, arg_info)->blockargc;
    unsigned int f_varargc = PTR_AS(YogArgInfo, arg_info)->varargc;
    unsigned int f_kwargc = PTR_AS(YogArgInfo, arg_info)->kwargc;
    YOG_ASSERT(env, (posargc <= f_argc) || (0 < f_varargc), "Too many argument(s).");
    unsigned int argc = f_argc + f_blockargc + f_varargc + f_kwargc;
    args = YogValArray_new(env, argc);
    if (0 < f_varargc) {
        YogVal vararg = YogArray_new(env);
        YogVal* items = PTR_AS(YogValArray, args)->items;
        unsigned int index = f_argc + f_blockargc;
        MODIFY(env, items[index], vararg);
    }

    fill_builtin_function_args(env, f, posargc, posargs, blockarg, kwargc, kwargs, vararg, varkwarg, args);

    YogVal self = PTR_AS(YogBuiltinBoundMethod, method)->self;
    YogVal retval = call_builtin_function(env, f, self, args);

    RETURN(env, retval);
}

#undef DECL_ARGS

static YogVal
make_outer_vars(YogEnv* env, unsigned int depth)
{
    YogVal outer_vars = YogOuterVars_new(env, depth);

    YogVal frame = CUR_FRAME;
    unsigned int i;
    for (i = 0; i < depth; i++) {
        YOG_ASSERT(env, IS_PTR(frame), "frame is not object");
        YOG_ASSERT(env, PTR_AS(YogFrame, frame)->type == FRAME_METHOD, "frame type isn't FRAME_METHOD");

        YogVal* items = PTR_AS(YogOuterVars, outer_vars)->items;
        items[i] = PTR_AS(YogMethodFrame, frame)->vars;

        frame = PTR_AS(YogFrame, frame)->prev;
    }

    return outer_vars;
}

static void 
setup_script_method(YogEnv* env, YogVal method, YogVal code) 
{
    SAVE_ARGS2(env, method, code);

    MODIFY(env, PTR_AS(YogScriptMethod, method)->code, code);
    MODIFY(env, PTR_AS(YogScriptMethod, method)->globals, SCRIPT_FRAME(CUR_FRAME)->globals);

    unsigned int outer_size = PTR_AS(YogCode, code)->outer_size;
    YogVal outer_vars = make_outer_vars(env, outer_size);

    MODIFY(env, PTR_AS(YogScriptMethod, method)->outer_vars, outer_vars);

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

static void 
call_code(YogEnv* env, YogVal self, YogVal code, uint8_t posargc, YogVal posargs[], YogVal blockarg, uint8_t kwargc, YogVal kwargs[], YogVal vararg, YogVal varkwarg)
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
    vars = YogValArray_new(env, local_vars_count);
    MODIFY(env, PTR_AS(YogValArray, vars)->items[0], self);

    YogVal arg_info = PTR_AS(YogCode, code)->arg_info;
    unsigned int code_argc = PTR_AS(YogArgInfo, arg_info)->argc;
    unsigned int code_blockargc = PTR_AS(YogArgInfo, arg_info)->blockargc;
    unsigned int code_varargc = PTR_AS(YogArgInfo, arg_info)->varargc;
    unsigned int code_kwargc = PTR_AS(YogArgInfo, arg_info)->kwargc;
    unsigned int argc = code_argc + code_blockargc + code_varargc + code_kwargc;
    fill_args(env, arg_info, posargc, posargs, blockarg, kwargc, kwargs, vararg, varkwarg, argc, vars, 1);

    frame = PTR2VAL(YogMethodFrame_new(env));
    setup_script_frame(env, frame, code);
    MODIFY(env, PTR_AS(YogMethodFrame, frame)->vars, vars);
    MODIFY(env, PTR_AS(YogScriptFrame, frame)->globals, SCRIPT_FRAME(CUR_FRAME)->globals);

    PUSH_FRAME(frame);

    RETURN_VOID(env);
}

#define PUSH(val)   YogScriptFrame_push_stack(env, SCRIPT_FRAME(CUR_FRAME), val)

static void 
call_method(YogEnv* env, YogVal unbound_self, YogVal callee, uint8_t posargc, YogVal posargs[], YogVal blockarg, uint8_t kwargc, YogVal kwargs[], YogVal vararg, YogVal varkwarg)
{
    SAVE_ARGS5(env, unbound_self, callee, blockarg, vararg, varkwarg);
#if 0
    PUSH_LOCALSX(env, posargc, posargs);
    PUSH_LOCALSX(env, 2 * kwargc, kwargs);
#endif

    YOG_ASSERT(env, IS_PTR(callee), "Callee is not object.");
    if (IS_OBJ_OF(cBuiltinBoundMethod, callee)) {
        YogVal val = call_builtin_bound_method(env, callee, posargc, posargs, blockarg, kwargc, kwargs, vararg, varkwarg);
        PUSH(val);
    }
    else if (IS_OBJ_OF(cBoundMethod, callee)) {
        YogVal self = PTR_AS(YogBoundMethod, callee)->self;
        YogVal code = PTR_AS(YogScriptMethod, callee)->code;
        DEBUG(DUMP_CODE(code));
        call_code(env, self, code, posargc, posargs, blockarg, kwargc, kwargs, vararg, varkwarg);
    }
    else if (IS_OBJ_OF(cBuiltinUnboundMethod, callee)) {
        YogVal self = unbound_self;
        YogVal val = call_builtin_unbound_method(env, self, callee, posargc, posargs, blockarg, kwargc, kwargs, vararg, varkwarg);
        PUSH(val);
    }
    else if (IS_OBJ_OF(cUnboundMethod, callee)) {
        YogVal self = unbound_self;
        YogVal code = PTR_AS(YogScriptMethod, callee)->code;
        call_code(env, self, code, posargc, posargs, blockarg, kwargc, kwargs, vararg, varkwarg);
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
                    YogVal vars = METHOD_FRAME(frame)->vars;
                    *val = PTR_AS(YogValArray, vars)->items[i];
                    return TRUE;
                }
            }
        }
        break;
    case FRAME_NAME:
        {
            YogVal vars = NAME_FRAME(frame)->vars;
            if (YogTable_lookup(env, vars, ID2VAL(name), val)) {
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
        const char* name = YogVM_id2name(env, vm, VAL2ID(key));
        YOG_BUG(env, "Can't find builtins attribute \"%s\".", name);
    }

    return val;
}

static YogVal
mainloop(YogEnv* env, YogVal frame, YogVal code) 
{
    SAVE_ARGS2(env, frame, code);

    DEBUG(DUMP_CODE(code));

    PUSH_FRAME(frame);

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
        jmpbuf.prev = PTR_AS(YogThread, env->thread)->jmp_buf_list;
        PTR_AS(YogThread, env->thread)->jmp_buf_list = &jmpbuf;
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

#define PRINT(...)  fprintf(stderr, __VA_ARGS__)
            PRINT("Traceback (most recent call last):\n");

            YogVal jmp_val = PTR_AS(YogThread, env->thread)->jmp_val;
            YogException* exc = PTR_AS(YogException, jmp_val);
            YogVal st = exc->stack_trace;
#define ID2NAME(id)     YogVM_id2name(env, env->vm, id)
            while (IS_PTR(st)) {
                PRINT("  File ");
                YogVal filename = PTR_AS(YogStackTraceEntry, st)->filename;
                if (IS_PTR(filename)) {
                    const char* name = PTR_AS(YogCharArray, filename)->items;
                    PRINT("\"%s\"", name);
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
            const char* name = ID2NAME(PTR_AS(YogKlass, klass)->name);
            /* dirty hack */
            size_t len = strlen(name);
            char s[len + 1];
            strcpy(s, name);
#undef ID2NAME
            YogVal val = YogEval_call_method(env, exc->message, "to_s", 0, NULL);
            YogString* msg = PTR_AS(YogString, val);
            PRINT("%s: %s\n", s, PTR_AS(YogCharArray, msg->body)->items);
#undef PRINT

            RETURN(env, INT2VAL(-1));
        }
    }

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
        OpCode op = PTR_AS(YogByteArray, CODE->insts)->items[PC];

#if 0
        do {
            printf("------------------------------ dump of stack for %p ------------------------------\n", env);
            YogVal stack = SCRIPT_FRAME(CUR_FRAME)->stack;
            unsigned int stack_size = SCRIPT_FRAME(CUR_FRAME)->stack_size;
            if (0 < stack_size) {
                unsigned int i;
                for (i = stack_size; 0 < i; i--) {
                    YogVal_print(env, PTR_AS(YogValArray, stack)->items[i - 1]);
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
#include "src/eval.inc"
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
eval_code(YogEnv* env, YogVal code, YogVal receiver, unsigned int argc, YogVal args[]) 
{
    SAVE_ARGS2(env, code, receiver);
#if 0
    PUSH_LOCALSX(env, argc, args);
#endif

    YogVal undef = YUNDEF;
    call_code(env, receiver, code, argc, args, undef, 0, NULL, undef, undef);

    YogVal retval = mainloop(env, CUR_FRAME, code);

    RETURN(env, retval);
}

static YogVal
eval_code2(YogEnv* env, YogVal code, YogVal receiver, unsigned int argc, YogVal args[], YogVal blockarg)
{
    SAVE_ARGS3(env, code, receiver, blockarg);

    YogVal undef = YUNDEF;
    call_code(env, receiver, code, argc, args, blockarg, 0, NULL, undef, undef);

    YogVal retval = mainloop(env, CUR_FRAME, code);

    RETURN(env, retval);
}

YogVal 
YogEval_call_block(YogEnv* env, YogVal block, unsigned int argc, YogVal* args) 
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
        vars = PTR_AS(YogPackageBlock, block)->vars;
        unsigned int argc = PTR_AS(YogArgInfo, arg_info)->argc;
        unsigned int i = 0;
        for (i = 0; i < argc; i++) {
            YogVal argnames = PTR_AS(YogArgInfo, arg_info)->argnames;
            ID name = PTR_AS(ID, argnames)[i];
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
        MODIFY(env, PTR_AS(YogNameFrame, frame)->self, self);
        MODIFY(env, PTR_AS(YogNameFrame, frame)->vars, vars);

        YogVal globals = YUNDEF;
        YogVal f = PTR_AS(YogFrame, CUR_FRAME)->prev;
        while (IS_PTR(f)) {
            YogFrameType type = PTR_AS(YogFrame, f)->type;
            if ((type == FRAME_METHOD) || (type == FRAME_NAME)) {
                globals = SCRIPT_FRAME(f)->globals;
                break;
            }

            f = PTR_AS(YogFrame, f)->prev;
        }
        YOG_ASSERT(env, IS_PTR(globals), "no globals");
        MODIFY(env, SCRIPT_FRAME(frame)->globals, globals);

        retval = mainloop(env, frame, code);

        POP_LOCALS(env);
    }
    else {
        YogVal frame = YUNDEF;
        YogVal code = YUNDEF;
        PUSH_LOCALS2(env, frame, code);

        frame = PTR2VAL(YogMethodFrame_new(env));
        code = PTR_AS(YogBasicBlock, block)->code;
        DEBUG(DUMP_CODE(code));
        setup_script_frame(env, frame, code);
        MODIFY(env, SCRIPT_FRAME(frame)->globals, PTR_AS(YogBlock, block)->globals);
        MODIFY(env, SCRIPT_FRAME(frame)->outer_vars, PTR_AS(YogBlock, block)->outer_vars);
        unsigned int local_vars_count = PTR_AS(YogCode, code)->local_vars_count;
        YogVal vars = YogValArray_new(env, local_vars_count);
        PTR_AS(YogMethodFrame, frame)->vars = vars;

        fill_args(env, PTR_AS(YogCode, code)->arg_info, argc, args, YUNDEF, 0, NULL, YUNDEF, YUNDEF, PTR_AS(YogValArray, vars)->size, vars, 1);

        retval = mainloop(env, frame, code);

        POP_LOCALS(env);
    }

    RETURN(env, retval);
}

YogVal 
YogEval_call_method_id(YogEnv* env, YogVal receiver, ID method, unsigned int argc, YogVal* args) 
{
    SAVE_ARG(env, receiver);
#if 0
    PUSH_LOCALSX(env, argc, args);
#endif

    YogVal attr = YogVal_get_attr(env, receiver, method);
    YOG_ASSERT(env, IS_PTR(attr), "Attribute isn't object.");

    YogVal retval = YUNDEF;
    YogVal undef = YUNDEF;
    if (IS_OBJ_OF(cBuiltinBoundMethod, attr)) {
        retval = call_builtin_bound_method(env, attr, argc, args, undef, 0, NULL, undef, undef);
    }
    else if (IS_OBJ_OF(cBoundMethod, attr)) {
        YogBoundMethod* method = PTR_AS(YogBoundMethod, attr);
        YogVal self = method->self;
        YogVal code = ((YogScriptMethod*)method)->code;
        retval = eval_code(env, code, self, argc, args);
    }
    else if (IS_OBJ_OF(cBuiltinUnboundMethod, attr)) {
        retval = call_builtin_unbound_method(env, receiver, attr, argc, args, YUNDEF, 0, NULL, undef, undef);
    }
    else if (IS_OBJ_OF(cUnboundMethod, attr)) {
        YogUnboundMethod* method = PTR_AS(YogUnboundMethod, attr);
        YogVal code = ((YogScriptMethod*)method)->code;
        retval = eval_code(env, code, receiver, argc, args);
    }
    else {
        YOG_ASSERT(env, FALSE, "Callee is not callable.");
    }

    RETURN(env, retval);
}

YogVal 
YogEval_call_method_id2(YogEnv* env, YogVal receiver, ID method, unsigned int argc, YogVal* args, YogVal blockarg)
{
    SAVE_ARGS2(env, receiver, blockarg);

    YogVal attr = YogVal_get_attr(env, receiver, method);
    YOG_ASSERT(env, IS_PTR(attr), "Attribute isn't object.");

    YogVal retval = YUNDEF;
    YogVal undef = YUNDEF;
    if (IS_OBJ_OF(cBuiltinBoundMethod, attr)) {
        retval = call_builtin_bound_method(env, attr, argc, args, blockarg, 0, NULL, undef, undef);
    }
    else if (IS_OBJ_OF(cBoundMethod, attr)) {
        YogBoundMethod* method = PTR_AS(YogBoundMethod, attr);
        YogVal self = method->self;
        YogVal code = ((YogScriptMethod*)method)->code;
        retval = eval_code2(env, code, self, argc, args, blockarg);
    }
    else if (IS_OBJ_OF(cBuiltinUnboundMethod, attr)) {
        retval = call_builtin_unbound_method(env, receiver, attr, argc, args, blockarg, 0, NULL, YUNDEF, YUNDEF);
    }
    else if (IS_OBJ_OF(cUnboundMethod, attr)) {
        YogUnboundMethod* method = PTR_AS(YogUnboundMethod, attr);
        YogVal code = ((YogScriptMethod*)method)->code;
        retval = eval_code2(env, code, receiver, argc, args, blockarg);
    }
    else {
        YOG_ASSERT(env, FALSE, "Callee is not callable.");
    }

    RETURN(env, retval);
}

static void
eval_package(YogEnv* env, YogVal pkg) 
{
    SAVE_ARG(env, pkg);

    YogVal frame = YUNDEF;
    YogVal code = YUNDEF;
    YogVal attrs = YUNDEF;
    PUSH_LOCALS3(env, frame, code, attrs);

    frame = YogPackageFrame_new(env);
    code = PTR_AS(YogPackage, pkg)->code;
    setup_script_frame(env, frame, code);
    MODIFY(env, PTR_AS(YogNameFrame, frame)->self, pkg);
    attrs = PTR_AS(YogObj, pkg)->attrs;
    MODIFY(env, PTR_AS(YogNameFrame, frame)->vars, attrs);
    MODIFY(env, SCRIPT_FRAME(frame)->globals, PTR_AS(YogNameFrame, frame)->vars);

    mainloop(env, frame, code);

    RETURN_VOID(env);
}

YogVal
YogEval_eval_file(YogEnv* env, const char* filename, const char* pkg_name)
{
    SAVE_LOCALS(env);

    YogVal stmts = YUNDEF;
    YogVal code = YUNDEF;
    YogVal pkg = YUNDEF;
    PUSH_LOCALS3(env, stmts, code, pkg);

    stmts = YogParser_parse_file(env, filename, FALSE);
    if (!IS_PTR(stmts)) {
        RETURN(env, YNIL);
    }
    code = YogCompiler_compile_module(env, filename, stmts);

    pkg = YogPackage_new(env);
    PTR_AS(YogPackage, pkg)->code = code;
    YogVM_register_package(env, env->vm, pkg_name, pkg);
    eval_package(env, pkg);

    RETURN(env, pkg);
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
