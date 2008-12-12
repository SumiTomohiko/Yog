#include <setjmp.h>
#include <stdio.h>
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
    FRAME(f)->prev = CUR_FRAME; \
    CUR_FRAME = FRAME(f); \
} while (0)

#define POP_FRAME()     do { \
    CUR_FRAME = CUR_FRAME->prev; \
} while (0)

YogVal 
YogThread_call_method(YogEnv* env, YogThread* th, YogVal receiver, const char* method, unsigned int argc, YogVal* args) 
{
    ID id = YogVm_intern(env, ENV_VM(env), method);
    YogVal retval = YogThread_call_method_id(env, th, receiver, id, argc, args);

    return retval;
}

static void 
fill_args(YogEnv* env, YogArgInfo* arg_info, uint8_t posargc, YogVal posargs[], YogVal blockarg, uint8_t kwargc, YogVal kwargs[], YogVal vararg, YogVal varkwarg, unsigned int argc, YogVal args[]) 
{
    unsigned int i = 0;
    for (i = 0; i < arg_info->argc + arg_info->blockargc; i++) {
        args[i] = YUNDEF;
    }

    if (arg_info->argc < posargc) {
        for (i = 0; i < arg_info->argc; i++) {
            args[i] = posargs[i];
        }
        YOG_ASSERT(env, arg_info->varargc == 1, "Too many arguments.");
        unsigned int index = arg_info->argc + arg_info->blockargc;
        YogArray* array = OBJ_AS(YogArray, args[index]);
        for (i = arg_info->argc; i < posargc; i++) {
            YogArray_push(env, array, posargs[i]);
        }
    }
    else {
        for (i = 0; i < posargc; i++) {
            args[i] = posargs[i];
        }
    }

    if (!IS_UNDEF(blockarg)) {
        YOG_ASSERT(env, arg_info->blockargc == 1, "Can't accept block argument.");
        unsigned int index = arg_info->argc;
        args[index] = blockarg;
    }

    for (i = 0; i < kwargc; i++) {
        YogVal name = kwargs[2 * i];
        ID id = VAL2ID(name);
        unsigned int j = 0;
        for (j = 0; j < arg_info->argc; j++) {
            ID argname = arg_info->argnames[j];
            if (argname == id) {
                YOG_ASSERT(env, !IS_UNDEF(args[j]), "Argument specified twice.");
                args[j] = kwargs[2 * i + 1];
                break;
            }
        }
        if (j == arg_info->argc) {
            ID argname = arg_info->blockargname;
            if (argname == id) {
                YOG_ASSERT(env, !IS_UNDEF(args[j]), "Argument specified twice.");
                args[argc - 1] = blockarg;
            }
        }
    }
}

static void 
fill_builtin_function_args(YogEnv* env, YogBuiltinFunction* f, uint8_t posargc, YogVal posargs[], YogVal blockarg, uint8_t kwargc, YogVal kwargs[], YogVal vararg, YogVal varkwarg, YogValArray* args)
{
    unsigned int argc = YogValArray_size(env, args);

    fill_args(env, &f->arg_info, posargc, posargs, blockarg, kwargc, kwargs, vararg, varkwarg, argc, args->items);

    int required_argc = 0;
    if (f->required_argc < 0) {
        required_argc = argc;
    }
    else {
        required_argc = f->required_argc;
    }
    unsigned int i = 0;
    for (i = 0; i < required_argc; i++) {
        YogVal val = YogValArray_at(env, args, i);
        YOG_ASSERT(env, !IS_UNDEF(val), "Argument not specified.");
    }
    for (i = required_argc; i < argc; i++) {
        YogVal val = YogValArray_at(env, args, i);
        if (IS_UNDEF(val)) {
            args->items[i] = YNIL;
        }
    }
}

static YogVal 
call_builtin_function(YogEnv* env, YogThread* th, YogBuiltinFunction* f, YogVal self, YogValArray* args)
{
    YogCFrame* frame = YogCFrame_new(env);
    frame->self = self;
    frame->args = args;
    frame->f = f;
    PUSH_FRAME(frame);

    YogVal retval = (*f->f)(env);

    POP_FRAME();

    return retval;
}

#define DECL_ARGS \
    YogBuiltinFunction* f = method->f; \
    YogArgInfo* arg_info = &f->arg_info; \
    YOG_ASSERT(env, (posargc <= arg_info->argc) || (0 < arg_info->varargc), "Too many argument(s)."); \
    unsigned int argc = arg_info->argc + arg_info->blockargc + arg_info->varargc + arg_info->kwargc; \
    YogValArray* args = YogValArray_new(env, argc); \
    if (0 < arg_info->varargc) { \
        YogArray* vararg = YogArray_new(env); \
        unsigned int index = arg_info->argc + arg_info->blockargc; \
        args->items[index] = OBJ2VAL(vararg); \
    }

static YogVal 
call_builtin_unbound_method(YogEnv* env, YogThread* th, YogVal receiver, YogBuiltinUnboundMethod* method, uint8_t posargc, YogVal posargs[], YogVal blockarg, uint8_t kwargc, YogVal kwargs[], YogVal vararg, YogVal varkwarg) 
{
    DECL_ARGS;

    fill_builtin_function_args(env, f, posargc, posargs, blockarg, kwargc, kwargs, vararg, varkwarg, args);

    YogVal retval = call_builtin_function(env, th, f, receiver, args);

    return retval;
}

static YogVal 
call_builtin_bound_method(YogEnv* env, YogThread* th, YogBuiltinBoundMethod* method, uint8_t posargc, YogVal posargs[], YogVal blockarg, uint8_t kwargc, YogVal kwargs[], YogVal vararg, YogVal varkwarg) 
{
    DECL_ARGS;

    fill_builtin_function_args(env, f, posargc, posargs, blockarg, kwargc, kwargs, vararg, varkwarg, args);

    YogVal self = method->self;
    YogVal retval = call_builtin_function(env, th, f, self, args);

    return retval;
}

#undef DECL_ARGS

static void 
setup_script_frame(YogEnv* env, YogScriptFrame* frame, YogCode* code) 
{
#if 0
    printf("%s:%d setup_script_frame(env=%p, frame=%p, code=%p)\n", __FILE__, __LINE__, env, frame, code);
    YogCode_dump(env, code);
#endif

    frame->pc = 0;
    frame->code = code;
    frame->stack = YogValArray_new(env, code->stack_size);
}

static void 
call_code(YogEnv* env, YogThread* th, YogVal self, YogCode* code, uint8_t posargc, YogVal posargs[], YogVal blockarg, uint8_t kwargc, YogVal kwargs[], YogVal vararg, YogVal varkwarg)
{
    YogValArray* vars = YogValArray_new(env, code->local_vars_count);
    vars->items[0] = self;

    YogArgInfo* arg_info = &code->arg_info;
    unsigned int argc = arg_info->argc + arg_info->blockargc + arg_info->varargc + arg_info->kwargc;
    fill_args(env, arg_info, posargc, posargs, blockarg, kwargc, kwargs, vararg, varkwarg, argc, &vars->items[1]);

    YogMethodFrame* frame = YogMethodFrame_new(env);
    setup_script_frame(env, SCRIPT_FRAME(frame), code);
    frame->vars = vars;

    PUSH_FRAME(frame);
}

#define PUSH(val)   YogScriptFrame_push_stack(env, SCRIPT_FRAME(CUR_FRAME), val)

static void 
call_method(YogEnv* env, YogThread* th, YogVal unbound_self, YogVal callee, uint8_t posargc, YogVal posargs[], YogVal blockarg, uint8_t kwargc, YogVal kwargs[], YogVal vararg, YogVal varkwarg)
{
    YOG_ASSERT(env, IS_OBJ(callee), "Callee is not object.");
    YogBasicObj* obj = VAL2OBJ(callee);
    YogVm* vm = ENV_VM(env);
    if (obj->klass == vm->cBuiltinBoundMethod) {
        YogBuiltinBoundMethod* method = (YogBuiltinBoundMethod*)obj;
        YogVal val = call_builtin_bound_method(env, th, method, posargc, posargs, blockarg, kwargc, kwargs, vararg, varkwarg);
        PUSH(val);
    }
    else if (obj->klass == vm->cBoundMethod) {
        YogBoundMethod* method = (YogBoundMethod*)obj;
        YogVal self = method->self;
        YogCode* code = method->code;
        call_code(env, th, self, code, posargc, posargs, blockarg, kwargc, kwargs, vararg, varkwarg);
    }
    else if (obj->klass == vm->cBuiltinUnboundMethod) {
        YogBuiltinUnboundMethod* method = (YogBuiltinUnboundMethod*)obj;
        YogVal self = unbound_self;
        YogVal val = call_builtin_unbound_method(env, th, self, method, posargc, posargs, blockarg, kwargc, kwargs, vararg, varkwarg);
        PUSH(val);
    }
    else if (obj->klass == vm->cUnboundMethod) {
        YogUnboundMethod* method = (YogUnboundMethod*)obj;
        YogVal self = unbound_self;
        YogCode* code = method->code;
        call_code(env, th, self, code, posargc, posargs, blockarg, kwargc, kwargs, vararg, varkwarg);
    }
    else {
        YOG_ASSERT(env, FALSE, "Callee is not callable.");
    }
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
mainloop(YogEnv* env, YogThread* th, YogScriptFrame* frame, YogCode* code) 
{
#if 0
    printf("%s:%d mainloop(env=%p, th=%p, frame=%p, code=%p)\n", __FILE__, __LINE__, env, th, frame, code);
    YogCode_dump(env, code);
#endif

    PUSH_FRAME(frame);

#define POP_BUF()   ENV_TH(env)->jmp_buf_list = ENV_TH(env)->jmp_buf_list->prev
#define PC          (SCRIPT_FRAME(CUR_FRAME)->pc)
#define CODE        (SCRIPT_FRAME(CUR_FRAME)->code)
    YogJmpBuf jmpbuf;
    int status = 0;
    if ((status = setjmp(jmpbuf.buf)) == 0) {
        jmpbuf.prev = ENV_TH(env)->jmp_buf_list;
        ENV_TH(env)->jmp_buf_list = &jmpbuf;
    }
    else {
        unsigned int i = 0;
        BOOL found = FALSE;
        if (CUR_FRAME->type != FRAME_C) {
            for (i = 0; i < CODE->exc_tbl_size; i++) {
                YogExceptionTableEntry* entry = &CODE->exc_tbl->items[i];
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
            YogStackTraceEntry* st = exc->stack_trace;
#define ID2NAME(id)     YogVm_id2name(env, ENV_VM(env), id)
            while (st != NULL) {
                PRINT("  File ");
                const char* filename = st->filename;
                if (filename != NULL) {
                    PRINT("\"%s\"", filename);
                }
                else {
                    PRINT("builtin");
                }

                unsigned int lineno = st->lineno;
                if (0 < lineno) {
                    PRINT(", line %d", lineno);
                }

                PRINT(", in ");
                ID klass_name = st->klass_name;
                ID func_name = st->func_name;
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

                st = st->lower;
            }

            YogKlass* klass = YOGBASICOBJ(exc)->klass;
            const char* name = ID2NAME(klass->name);
#undef ID2NAME
            YogVal val = YogThread_call_method(env, ENV_TH(env), exc->message, "to_s", 0, NULL);
            YogString* msg = OBJ_AS(YogString, val);
            PRINT("%s: %s\n", name, msg->body->items);
#undef PRINT

            return INT2VAL(-1);
        }
    }

    while (PC < CODE->insts->size) {
#define ENV             (env)
#define VM              (ENV_VM(ENV))
        th = VM->thread;

#define POP()           (YogScriptFrame_pop_stack(env, SCRIPT_FRAME(CUR_FRAME)))
#define CONSTS(index)   (YogValArray_at(env, CODE->consts, index))
#define THREAD          (ENV_TH(env))
#define JUMP(m)         PC = m;
#define POP_ARGS(args, kwargs, blockarg, vararg, varkwarg) \
    YogVal varkwarg = YUNDEF; \
    if (varkwargc == 1) { \
        varkwarg = POP(); \
    } \
\
    YogVal vararg = YUNDEF; \
    if (varargc == 1) { \
        vararg = POP(); \
    } \
\
    YogVal blockarg = YUNDEF; \
    if (blockargc == 1) { \
        blockarg = POP(); \
    } \
\
    YogVal kwargs[2 * kwargc]; \
    unsigned int i = 0; \
    for (i = kwargc; 0 < i; i--) { \
        kwargs[2 * i - 1] = POP(); \
        kwargs[2 * i - 2] = POP(); \
    } \
\
    YogVal args[argc]; \
    for (i = argc; 0 < i; i--) { \
        args[i - 1] = POP(); \
    }
#if 0
        if (0 < STACK->size) {
            YogVal_print(env, STACK->items[STACK->size - 1]);
        }
        else {
            printf("stack is empty.\n");
        }
#endif

        OpCode op = CODE->insts->items[PC];
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

    return YUNDEF;
}

static YogVal 
eval_code(YogEnv* env, YogThread* th, YogCode* code, YogVal receiver, unsigned int argc, YogVal args[]) 
{
    YogVal undef = YUNDEF;
    call_code(env, th, receiver, code, argc, args, undef, 0, NULL, undef, undef);

    YogVal retval = mainloop(env, th, SCRIPT_FRAME(CUR_FRAME), code);

    return retval;
}

YogVal 
YogThread_call_block(YogEnv* env, YogThread* th, YogVal block, unsigned int argc, YogVal* args) 
{
    YogBasicObj* obj = VAL2OBJ(block);

    YogVal retval = YUNDEF;
    if (obj->klass == ENV_VM(env)->cPackageBlock) {
        YogBlock* block = BLOCK(obj);
        YogCode* code = block->code;
        YogArgInfo* arg_info = &code->arg_info;

#define SET_VAR(name) do { \
    YogVal symbol = ID2VAL(name); \
    YogTable_insert(env, vars, symbol, args[i]); \
} while (0)
        YogPackageBlock* pkg_block = PACKAGE_BLOCK(obj);
        YogTable* vars = pkg_block->vars;
        unsigned int i = 0;
        for (i = 0; i < arg_info->argc; i++) {
            ID name = arg_info->argnames[i];
            SET_VAR(name);
        }
        if (i < arg_info->argc + arg_info->blockargc) {
            ID name = arg_info->blockargname;
            SET_VAR(name);
            i++;
        }
#undef SET_VAR

        YogPackageFrame* frame = YogPackageFrame_new(env);
        setup_script_frame(env, SCRIPT_FRAME(frame), code);
        NAME_FRAME(frame)->self = pkg_block->self;
        NAME_FRAME(frame)->vars = vars;

        YogVal retval = mainloop(env, th, SCRIPT_FRAME(frame), code);

        return retval;
    }
    else {
        YOG_ASSERT(env, FALSE, "Block class isn't PackageBlock.");
    }

    return retval;
}

YogVal 
YogThread_call_method_id(YogEnv* env, YogThread* th, YogVal receiver, ID method, unsigned int argc, YogVal* args) 
{
    YogVal attr = YogVal_get_attr(env, receiver, method);
    YOG_ASSERT(env, IS_OBJ(attr), "Attribute isn't object.");
    YogBasicObj* obj = VAL2OBJ(attr);

    YogVal retval = YUNDEF;
    YogVal undef = YUNDEF;
    if (obj->klass == ENV_VM(env)->cBuiltinBoundMethod) {
        YogBuiltinBoundMethod* method = (YogBuiltinBoundMethod*)obj;
        retval = call_builtin_bound_method(env, th, method, argc, args, undef, 0, NULL, undef, undef);
    }
    else if (obj->klass == ENV_VM(env)->cBoundMethod) {
        YogBoundMethod* method = (YogBoundMethod*)obj;
        YogVal self = method->self;
        YogCode* code = method->code;
        retval = eval_code(env, th, code, self, argc, args);
    }
    else if (obj->klass == ENV_VM(env)->cBuiltinUnboundMethod) {
        YogBuiltinUnboundMethod* method = (YogBuiltinUnboundMethod*)obj;
        retval = call_builtin_unbound_method(env, th, receiver, method, argc, args, undef, 0, NULL, undef, undef);
    }
    else if (obj->klass == ENV_VM(env)->cUnboundMethod) {
        YogUnboundMethod* method = (YogUnboundMethod*)obj;
        YogCode* code = method->code;
        retval = eval_code(env, th, code, receiver, argc, args);
    }
    else {
        YOG_ASSERT(env, FALSE, "Callee is not callable.");
    }

    return retval;
}

void 
YogThread_eval_package(YogEnv* env, YogThread* th, YogPackage* pkg)
{
    YogCode* code = pkg->code;

    YogPackageFrame* frame = YogPackageFrame_new(env);
    setup_script_frame(env, SCRIPT_FRAME(frame), code);
    NAME_FRAME(frame)->self = OBJ2VAL(pkg);
    NAME_FRAME(frame)->vars = YOGOBJ(pkg)->attrs;

    mainloop(env, th, SCRIPT_FRAME(frame), code);
}

static void 
keep_children(YogEnv* env, void* ptr, ObjectKeeper keeper)
{
    YogThread* th = ptr;

    th->cur_frame = (*keeper)(env, th->cur_frame);
    th->jmp_val = YogVal_keep(env, th->jmp_val, keeper);
}

YogThread*
YogThread_new(YogEnv* env) 
{
    YogThread* th = ALLOC_OBJ(env, keep_children, NULL, YogThread);
    th->cur_frame = NULL;
    th->jmp_buf_list = NULL;
    th->jmp_val = YUNDEF;

    return th;
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
