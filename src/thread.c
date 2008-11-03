#include <setjmp.h>
#include <stdio.h>
#include "yog/opcodes.h"
#include "yog/yog.h"

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
        args[i] = YogVal_undef();
    }

    if (arg_info->argc < posargc) {
        for (i = 0; i < arg_info->argc; i++) {
            args[i] = posargs[i];
        }
        Yog_assert(env, arg_info->varargc == 1, "Too many arguments.");
        unsigned int index = arg_info->argc + arg_info->blockargc;
        YogArray* array = (YogArray*)YOGVAL_OBJ(args[index]);
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
        Yog_assert(env, arg_info->blockargc == 1, "Can't accept block argument.");
        unsigned int index = arg_info->argc;
        args[index] = blockarg;
    }

    for (i = 0; i < kwargc; i++) {
        YogVal name = kwargs[2 * i];
        ID id = YOGVAL_SYMBOL(name);
        unsigned int j = 0;
        for (j = 0; j < arg_info->argc; j++) {
            ID argname = arg_info->argnames[j];
            if (argname == id) {
                Yog_assert(env, !IS_UNDEF(args[j]), "Argument specified twice.");
                args[j] = kwargs[2 * i + 1];
                break;
            }
        }
        if (j == arg_info->argc) {
            ID argname = arg_info->blockargname;
            if (argname == id) {
                Yog_assert(env, !IS_UNDEF(args[j]), "Argument specified twice.");
                args[argc - 1] = blockarg;
            }
        }
    }
}

static void 
fill_builtin_function_args(YogEnv* env, YogBuiltinFunction* f, uint8_t posargc, YogVal posargs[], YogVal blockarg, uint8_t kwargc, YogVal kwargs[], YogVal vararg, YogVal varkwarg, unsigned int argc, YogVal args[])
{
    fill_args(env, &f->arg_info, posargc, posargs, blockarg, kwargc, kwargs, vararg, varkwarg, argc, args);

    int required_argc = 0;
    if (f->required_argc < 0) {
        required_argc = argc;
    }
    else {
        required_argc = f->required_argc;
    }
    unsigned int i = 0;
    for (i = 0; i < required_argc; i++) {
        YogVal val = args[i];
        Yog_assert(env, !IS_UNDEF(val), "Argument not specified.");
    }
    for (i = required_argc; i < argc; i++) {
        YogVal val = args[i];
        if (IS_UNDEF(val)) {
            args[i] = YogVal_nil();
        }
    }
}

static YogVal 
call_builtin_function(YogEnv* env, YogBuiltinFunction* f, YogVal self, YogVal args[]) 
{
    YogArgInfo* arg_info = &f->arg_info;
    uint8_t argc = arg_info->argc + arg_info->blockargc;
    uint8_t varargc = arg_info->varargc;
    uint8_t kwargc = arg_info->kwargc;

#include "src/call_builtin_function.inc"

    /* NOTREACHED */
    return YogVal_nil();
}

#define DECL_ARGS \
    YogBuiltinFunction* f = method->f; \
    YogArgInfo* arg_info = &f->arg_info; \
    Yog_assert(env, (posargc <= arg_info->argc) || (0 < arg_info->varargc), "Too many argument(s)."); \
    unsigned int argc = arg_info->argc + arg_info->blockargc + arg_info->varargc + arg_info->kwargc; \
    YogVal args[argc]; \
    if (0 < arg_info->varargc) { \
        YogArray* vararg = YogArray_new(env); \
        unsigned int index = arg_info->argc + arg_info->blockargc; \
        args[index] = YogVal_obj((YogBasicObj*)vararg); \
    }

static YogVal 
call_builtin_unbound_method(YogEnv* env, YogVal receiver, YogBuiltinUnboundMethod* method, uint8_t posargc, YogVal posargs[], YogVal blockarg, uint8_t kwargc, YogVal kwargs[], YogVal vararg, YogVal varkwarg) 
{
    DECL_ARGS;

    fill_builtin_function_args(env, f, posargc, posargs, blockarg, kwargc, kwargs, vararg, varkwarg, argc, args);

    YogVal retval = call_builtin_function(env, f, receiver, args);

    return retval;
}

static YogVal 
call_builtin_bound_method(YogEnv* env, YogBuiltinBoundMethod* method, uint8_t posargc, YogVal posargs[], YogVal blockarg, uint8_t kwargc, YogVal kwargs[], YogVal vararg, YogVal varkwarg) 
{
    DECL_ARGS;

    fill_builtin_function_args(env, f, posargc, posargs, blockarg, kwargc, kwargs, vararg, varkwarg, argc, args);

    YogVal self = method->self;
    YogVal retval = call_builtin_function(env, f, self, args);

    return retval;
}

#undef DECL_ARGS

static void 
call_code(YogEnv* env, YogThread* th, YogVal self, YogCode* code, uint8_t posargc, YogVal posargs[], YogVal blockarg, uint8_t kwargc, YogVal kwargs[], YogVal vararg, YogVal varkwarg)
{
    YogValArray* vars = YogValArray_new(env, code->local_vars_count);
    vars->items[0] = self;

    YogArgInfo* arg_info = &code->arg_info;
    unsigned int argc = arg_info->argc + arg_info->blockargc + arg_info->varargc + arg_info->kwargc;
    fill_args(env, arg_info, posargc, posargs, blockarg, kwargc, kwargs, vararg, varkwarg, argc, &vars->items[1]);

    YogMethodFrame* frame = YogMethodFrame_new(env);
    SCRIPT_FRAME(frame)->stack = YogValArray_new(env, code->stack_size);
    frame->vars = vars;

    th->cur_frame = FRAME(frame);
}

#define CUR_FRAME   (th->cur_frame)
#define STACK       (SCRIPT_FRAME(CUR_FRAME)->stack)
#define PUSH(val)   (YogValArray_push(env, STACK, val))

static void 
call_method(YogEnv* env, YogThread* th, YogVal unbound_self, YogVal callee, uint8_t posargc, YogVal posargs[], YogVal blockarg, uint8_t kwargc, YogVal kwargs[], YogVal vararg, YogVal varkwarg)
{
    Yog_assert(env, IS_OBJ(callee), "Callee is not object.");
    YogBasicObj* obj = YOGVAL_OBJ(callee);
    YogVm* vm = ENV_VM(env);
    if (obj->klass == vm->builtin_bound_method_klass) {
        YogBuiltinBoundMethod* method = (YogBuiltinBoundMethod*)obj;
        YogVal val = call_builtin_bound_method(env, method, posargc, posargs, blockarg, kwargc, kwargs, vararg, varkwarg);
        PUSH(val);
    }
    else if (obj->klass == vm->bound_method_klass) {
        YogBoundMethod* method = (YogBoundMethod*)obj;
        YogVal self = method->self;
        YogCode* code = method->code;
        call_code(env, th, self, code, posargc, posargs, blockarg, kwargc, kwargs, vararg, varkwarg);
    }
    else if (obj->klass == vm->builtin_unbound_method_klass) {
        YogBuiltinUnboundMethod* method = (YogBuiltinUnboundMethod*)obj;
        YogVal self = unbound_self;
        YogVal val = call_builtin_unbound_method(env, self, method, posargc, posargs, blockarg, kwargc, kwargs, vararg, varkwarg);
        PUSH(val);
    }
    else if (obj->klass == vm->unbound_method_klass) {
        YogUnboundMethod* method = (YogUnboundMethod*)obj;
        YogVal self = unbound_self;
        YogCode* code = method->code;
        call_code(env, th, self, code, posargc, posargs, blockarg, kwargc, kwargs, vararg, varkwarg);
    }
    else {
        Yog_assert(env, FALSE, "Callee is not callable.");
    }
}

static YogVal
mainloop(YogEnv* env, YogThread* th, YogScriptFrame* frame, YogCode* code) 
{
    th->cur_frame = FRAME(frame);

#define POP_BUF()   th->jmp_buf_list = th->jmp_buf_list->prev
#define PC          (SCRIPT_FRAME(CUR_FRAME)->pc)
    YogJmpBuf jmpbuf;
    int status = 0;
    if ((status = setjmp(jmpbuf.buf)) == 0) {
        jmpbuf.prev = th->jmp_buf_list;
        th->jmp_buf_list = &jmpbuf;
    }
    else {
        unsigned int i = 0;
        for (i = 0; i < code->exc_tbl_size; i++) {
            YogExcTblEntry* entry = &code->exc_tbl->items[i];
            BOOL found = FALSE;
            if ((entry->from <= PC) && (PC < entry->to)) {
                PC = entry->target;
                found = TRUE;
                break;
            }
            if (!found) {
                POP_BUF();
                Yog_assert(env, th->jmp_buf_list != NULL, "No more jmp_buf.");
                longjmp(th->jmp_buf_list->buf, status);
            }
        }
    }

    while (PC < code->insts->size) {
#if 0
        printf("%s:%d pc=%d\n", __FILE__, __LINE__, pc);
#endif
#define CODE            (code)
#define POP()           (YogValArray_pop(env, STACK))
#define CONSTS(index)   (YogValArray_at(env, code->consts, index))
#define ENV             (env)
#define VM              (ENV_VM(ENV))
#define THREAD          (th)
#define JUMP(m)         \
    PC = m; \
    continue
#define POP_ARGS(args, kwargs, blockarg, vararg, varkwarg) \
    YogVal varkwarg = YogVal_undef(); \
    if (varkwargc == 1) { \
        varkwarg = POP(); \
    } \
\
    YogVal vararg = YogVal_undef(); \
    if (varargc == 1) { \
        vararg = POP(); \
    } \
\
    YogVal blockarg = YogVal_undef(); \
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

        OpCode op = code->insts->items[PC];
        unsigned int n = PC + sizeof(uint8_t);
        switch (op) {
#include "src/thread.inc"
        default:
            Yog_assert(env, FALSE, "Unknown instruction.");
            break;
        }
#undef POP_ARGS
#undef JUMP
#undef THREAD
#undef VM
#undef ENV
#undef CONSTS
#undef POP
#undef CODE

        PC = n;
    }

    POP_BUF();
#undef PC
#undef POP_BUF

    return YogVal_undef();
}

static YogVal 
eval_code(YogEnv* env, YogThread* th, YogCode* code, YogVal receiver, unsigned int argc, YogVal args[]) 
{
    YogCFrame* frame = YogCFrame_new(env);
    th->cur_frame = frame;

    YogVal undef = YogVal_undef();
    call_code(env, th, receiver, code, argc, args, undef, 0, NULL, undef, undef);

    YogVal retval = mainloop(env, th, SCRIPT_FRAME(th->cur_frame), code);

    return retval;
}

YogVal 
YogThread_call_method_id(YogEnv* env, YogThread* th, YogVal receiver, ID method, unsigned int argc, YogVal* args) 
{
    YogKlass* klass = YogVal_get_klass(env, receiver);
    YogVal attr = YogObj_get_attr(env, YOGOBJ(klass), method);
    Yog_assert(env, IS_OBJ(attr), "Attribute isn't object.");
    YogBasicObj* obj = YOGVAL_OBJ(attr);

    YogVal retval = YogVal_undef();
    YogVal undef = YogVal_undef();
    if (obj->klass == ENV_VM(env)->builtin_bound_method_klass) {
        YogBuiltinBoundMethod* method = (YogBuiltinBoundMethod*)obj;
        retval = call_builtin_bound_method(env, method, argc, args, undef, 0, NULL, undef, undef);
    }
    else if (obj->klass == ENV_VM(env)->bound_method_klass) {
        YogBoundMethod* method = (YogBoundMethod*)obj;
        YogVal self = method->self;
        YogCode* code = method->code;
        retval = eval_code(env, th, code, self, argc, args);
    }
    else if (obj->klass == ENV_VM(env)->builtin_unbound_method_klass) {
        YogBuiltinUnboundMethod* method = (YogBuiltinUnboundMethod*)obj;
        retval = call_builtin_unbound_method(env, receiver, method, argc, args, undef, 0, NULL, undef, undef);
    }
    else if (obj->klass == ENV_VM(env)->unbound_method_klass) {
        YogUnboundMethod* method = (YogUnboundMethod*)obj;
        YogCode* code = method->code;
        retval = eval_code(env, th, code, receiver, argc, args);
    }
    else {
        Yog_assert(env, FALSE, "Callee is not callable.");
    }

    return retval;
}

void 
YogThread_call_command(YogEnv* env, ID command, unsigned int argc, YogVal* args)
{
#if 0
    YogVm* vm = ENV_VM(env);
    ID bltins = YogVm_intern(env, vm, "builtins");
    YogVal pkg = YogVal_undef();
    if (!YogTable_lookup(env, vm->pkgs, YogVal_symbol(bltins), &pkg)) {
        Yog_assert(env, FALSE, "Can't find builtins package.");
    }
    YogVal attr = YogObj_get_attr(env, YOGVAL_PTR(pkg), command);
    Yog_assert(env, YOGVAL_TYPE(attr) == VAL_FUNC, "Command isn't a function.");
    (YOGVAL_FUNC(attr))(env, pkg, argc, args);
#endif
}

void 
YogThread_eval_package(YogEnv* env, YogThread* th, YogPkg* pkg, YogCode* code) 
{
#if 0
    YogCode_dump(env, code);
#endif

    YogPkgFrame* frame = YogPkgFrame_new(env);
    SCRIPT_FRAME(frame)->stack = YogValArray_new(env, code->stack_size);
    frame->vars = pkg->attrs;

    mainloop(env, th, SCRIPT_FRAME(frame), code);
}

static void 
gc_children(YogEnv* env, void* ptr, DoGc do_gc) 
{
    YogThread* th = ptr;
    th->cur_frame = do_gc(env, th->cur_frame);
}

YogThread*
YogThread_new(YogEnv* env) 
{
    YogThread* th = ALLOC_OBJ(env, gc_children, YogThread);
    th->cur_frame = NULL;
    th->jmp_buf_list = NULL;
    th->jmp_val = YogVal_undef();

    return th;
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
