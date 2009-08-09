#include "yog/arg.h"
#include "yog/array.h"
#include "yog/code.h"
#include "yog/error.h"
#include "yog/eval.h"
#include "yog/frame.h"
#include "yog/function.h"
#include "yog/klass.h"
#include "yog/thread.h"
#include "yog/yog.h"

static void
fill_args(YogEnv* env, YogVal arg_info, uint8_t posargc, YogVal posargs[], YogVal blockarg, uint8_t kwargc, YogVal kwargs[], YogVal vararg, YogVal varkwarg, uint_t argc, YogVal args, uint_t args_offset)
{
    SAVE_ARGS5(env, arg_info, blockarg, vararg, varkwarg, args);

    YogVal array = YUNDEF;
    PUSH_LOCAL(env, array);

    uint_t i = 0;
    uint_t arg_argc = PTR_AS(YogArgInfo, arg_info)->argc;
    uint_t arg_blockargc = PTR_AS(YogArgInfo, arg_info)->blockargc;
    uint_t size = arg_argc + arg_blockargc;
    for (i = 0; i < size; i++) {
        PTR_AS(YogValArray, args)->items[args_offset + i] = YUNDEF;
    }

    if (PTR_AS(YogArgInfo, arg_info)->argc < posargc) {
        for (i = 0; i < PTR_AS(YogArgInfo, arg_info)->argc; i++) {
            YogVal* items = PTR_AS(YogValArray, args)->items;
            YogVal arg = posargs[i];
            items[args_offset + i] = arg;
        }
        YOG_ASSERT(env, PTR_AS(YogArgInfo, arg_info)->varargc == 1, "Too many arguments.");
        uint_t argc = PTR_AS(YogArgInfo, arg_info)->argc;
        uint_t blockargc = PTR_AS(YogArgInfo, arg_info)->blockargc;
        uint_t index = argc + blockargc;
        array = PTR_AS(YogValArray, args)->items[args_offset + index];
        for (i = PTR_AS(YogArgInfo, arg_info)->argc; i < posargc; i++) {
            YogArray_push(env, array, posargs[i]);
        }
    }
    else {
        for (i = 0; i < posargc; i++) {
            YogVal* items = PTR_AS(YogValArray, args)->items;
            YogVal arg = posargs[i];
            items[args_offset + i] = arg;
        }
    }

    if (IS_PTR(blockarg)) {
        YOG_ASSERT(env, PTR_AS(YogArgInfo, arg_info)->blockargc == 1, "Can't accept block argument.");
        YogVal* items = PTR_AS(YogValArray, args)->items;
        uint_t index = PTR_AS(YogArgInfo, arg_info)->argc;
        items[args_offset + index] = blockarg;
    }

    for (i = 0; i < kwargc; i++) {
        YogVal name = kwargs[2 * i];
        ID id = VAL2ID(name);
        uint_t j = 0;
        for (j = 0; j < PTR_AS(YogArgInfo, arg_info)->argc; j++) {
            YogVal argnames = PTR_AS(YogArgInfo, arg_info)->argnames;
            ID argname = PTR_AS(ID, argnames)[j];
            if (argname == id) {
                YOG_ASSERT(env, !IS_UNDEF(PTR_AS(YogValArray, args)->items[j]), "Argument specified twice.");
                YogVal* items = PTR_AS(YogValArray, args)->items;
                YogVal val = kwargs[2 * i + 1];
                items[args_offset + j] = val;
                break;
            }
        }
        if (j == PTR_AS(YogArgInfo, arg_info)->argc) {
            ID argname = PTR_AS(YogArgInfo, arg_info)->blockargname;
            if (argname == id) {
                YOG_ASSERT(env, !IS_UNDEF(PTR_AS(YogValArray, args)->items[args_offset + j]), "Argument specified twice.");
                YogVal* items = PTR_AS(YogValArray, args)->items;
                items[args_offset + argc - 1] = blockarg;
            }
        }
    }

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

    uint_t stack_size = PTR_AS(YogCode, code)->stack_size;
    YogVal stack = YogValArray_new(env, stack_size);

    PTR_AS(YogScriptFrame, frame)->pc = 0;
    PTR_AS(YogScriptFrame, frame)->code = code;
    PTR_AS(YogScriptFrame, frame)->stack = stack;

    RETURN_VOID(env);
}

static void
YogFunction_keep_children(YogEnv* env, void* ptr, ObjectKeeper keeper, void* heap)
{
    YogBasicObj_keep_children(env, ptr, keeper, heap);

    YogFunction* f = ptr;
#define KEEP(member)    YogGC_keep(env, &f->member, keeper, heap)
    KEEP(code);
    KEEP(globals);
    KEEP(outer_vars);
#undef KEEP
}

static void
YogFunction_exec_for_instance(YogEnv* env, YogVal callee, YogVal self, uint8_t posargc, YogVal posargs[], YogVal blockarg, uint8_t kwargc, YogVal kwargs[], YogVal vararg, YogVal varkwarg)
{
    SAVE_ARGS5(env, callee, self, blockarg, vararg, varkwarg);
    YogVal code = YUNDEF;
    YogVal outer_vars = YUNDEF;
    YogVal vars = YUNDEF;
    YogVal frame = YUNDEF;
    PUSH_LOCALS4(env, code, outer_vars, vars, frame);

    code = PTR_AS(YogFunction, callee)->code;
    outer_vars = PTR_AS(YogFunction, callee)->outer_vars;

    uint_t local_vars_count = PTR_AS(YogCode, code)->local_vars_count;
    vars = YogValArray_new(env, local_vars_count);
    PTR_AS(YogValArray, vars)->items[0] = self;

    YogVal arg_info = PTR_AS(YogCode, code)->arg_info;
    if (IS_PTR(arg_info)) {
        uint_t code_argc = PTR_AS(YogArgInfo, arg_info)->argc;
        uint_t code_blockargc = PTR_AS(YogArgInfo, arg_info)->blockargc;
        uint_t code_varargc = PTR_AS(YogArgInfo, arg_info)->varargc;
        uint_t code_kwargc = PTR_AS(YogArgInfo, arg_info)->kwargc;
        uint_t argc = code_argc + code_blockargc + code_varargc + code_kwargc;
        fill_args(env, arg_info, posargc, posargs, blockarg, kwargc, kwargs, vararg, varkwarg, argc, vars, 1);
    }

    frame = YogMethodFrame_new(env);
    setup_script_frame(env, frame, code);
    PTR_AS(YogMethodFrame, frame)->vars = vars;
    PTR_AS(YogScriptFrame, frame)->globals = PTR_AS(YogFunction, callee)->globals;
    PTR_AS(YogScriptFrame, frame)->outer_vars = outer_vars;

    PTR_AS(YogFrame, frame)->prev = PTR_AS(YogThread, env->thread)->cur_frame;
    PTR_AS(YogThread, env->thread)->cur_frame = frame;

    RETURN_VOID(env);
}

static void
YogFunction_exec(YogEnv* env, YogVal callee, uint8_t posargc, YogVal posargs[], YogVal blockarg, uint8_t kwargc, YogVal kwargs[], YogVal vararg, YogVal varkwarg)
{
    YogFunction_exec_for_instance(env, callee, YUNDEF, posargc, posargs, blockarg, kwargc, kwargs, vararg, varkwarg);
}

static YogVal
YogFunction_call_for_instance(YogEnv* env, YogVal callee, YogVal self, uint8_t posargc, YogVal posargs[], YogVal blockarg, uint8_t kwargc, YogVal kwargs[], YogVal vararg, YogVal varkwarg)
{
    SAVE_ARGS5(env, callee, self, blockarg, vararg, varkwarg);
    YogVal retval = YUNDEF;
    PUSH_LOCAL(env, retval);

    YogEval_push_finish_frame(env);

    YogFunction_exec_for_instance(env, callee, self, posargc, posargs, blockarg, kwargc, kwargs, vararg, varkwarg);
    retval = YogEval_mainloop(env);

    RETURN(env, retval);
}

static YogVal
YogFunction_call(YogEnv* env, YogVal callee, uint8_t posargc, YogVal posargs[], YogVal blockarg, uint8_t kwargc, YogVal kwargs[], YogVal vararg, YogVal varkwarg)
{
    return YogFunction_call_for_instance(env, callee, YUNDEF, posargc, posargs, blockarg, kwargc, kwargs, vararg, varkwarg);
}

static void
YogFunction_initialize(YogEnv* env, YogVal self, YogVal klass)
{
    YogBasicObj_init(env, self, 0, klass);

    PTR_AS(YogFunction, self)->code = YUNDEF;
    PTR_AS(YogFunction, self)->globals = YUNDEF;
    PTR_AS(YogFunction, self)->outer_vars = YUNDEF;
}

static YogVal
YogFunction_allocate(YogEnv* env, YogVal klass)
{
    SAVE_ARG(env, klass);
    YogVal f = YUNDEF;
    PUSH_LOCAL(env, f);

    f = ALLOC_OBJ(env, YogFunction_keep_children, NULL, YogFunction);
    YogFunction_initialize(env, f, klass);

    RETURN(env, f);
}

static YogVal
YogFunction_call_get_descr(YogEnv* env, YogVal self, YogVal obj, YogVal klass)
{
    SAVE_ARGS3(env, self, obj, klass);
    YogVal val = YUNDEF;
    PUSH_LOCAL(env, val);

    val = YogInstanceMethod_new(env);
    PTR_AS(YogInstanceMethod, val)->self = obj;
    PTR_AS(YogInstanceMethod, val)->f = self;

    RETURN(env, val);
}

static void
YogFunction_exec_get_descr(YogEnv* env, YogVal self, YogVal obj, YogVal klass)
{
    SAVE_ARGS3(env, self, obj, klass);
    YogVal retval = YUNDEF;
    PUSH_LOCAL(env, retval);

    retval = YogFunction_call_get_descr(env, self, obj, klass);
    FRAME_PUSH(env, retval);

    RETURN_VOID(env);
}

YogVal
YogFunction_klass_new(YogEnv* env)
{
    SAVE_LOCALS(env);

    YogVal klass = YUNDEF;
    PUSH_LOCAL(env, klass);

    klass = YogKlass_new(env, "Function", env->vm->cObject);
    YogKlass_define_allocator(env, klass, YogFunction_allocate);
    YogKlass_define_descr_get_executor(env, klass, YogFunction_exec_get_descr);
    YogKlass_define_descr_get_caller(env, klass, YogFunction_call_get_descr);
    YogKlass_define_caller(env, klass, YogFunction_call);
    YogKlass_define_executor(env, klass, YogFunction_exec);

    RETURN(env, klass);
}

YogVal
YogFunction_new(YogEnv* env)
{
    return YogFunction_allocate(env, env->vm->cFunction);
}

static YogVal
YogNativeFunction_call_for_instance(YogEnv* env, YogVal callee, YogVal self, uint8_t posargc, YogVal posargs[], YogVal blockarg, uint8_t kwargc, YogVal kwargs[], YogVal vararg, YogVal varkwarg)
{
    SAVE_ARGS5(env, callee, self, blockarg, vararg, varkwarg);
    YogVal args = YUNDEF;
    YogVal retval = YUNDEF;
    YogVal frame = YUNDEF;
    PUSH_LOCALS3(env, args, retval, frame);

    args = YogArray_new(env);
    uint_t i;
    for (i = 0; i < posargc; i++) {
        YogArray_push(env, args, posargs[i]);
    }

    frame = YogCFrame_new(env);
    PTR_AS(YogCFrame, frame)->f = callee;
    PTR_AS(YogFrame, frame)->prev = PTR_AS(YogThread, env->thread)->cur_frame;
    PTR_AS(YogThread, env->thread)->cur_frame = frame;

    YogVal (*f)(YogEnv*, YogVal, YogVal, YogVal, YogVal) = PTR_AS(YogNativeFunction, callee)->f;
    retval = (*f)(env, self, args, YUNDEF, blockarg);

    PTR_AS(YogThread, env->thread)->cur_frame = PTR_AS(YogFrame, PTR_AS(YogThread, env->thread)->cur_frame)->prev;

    RETURN(env, retval);
}

static YogVal
YogNativeFunction_call(YogEnv* env, YogVal callee, uint8_t posargc, YogVal posargs[], YogVal blockarg, uint8_t kwargc, YogVal kwargs[], YogVal vararg, YogVal varkwarg)
{
    return YogNativeFunction_call_for_instance(env, callee, YUNDEF, posargc, posargs, blockarg, kwargc, kwargs, vararg, varkwarg);
}

static void
YogNativeFunction_exec_for_instance(YogEnv* env, YogVal callee, YogVal self, uint8_t posargc, YogVal posargs[], YogVal blockarg, uint8_t kwargc, YogVal kwargs[], YogVal vararg, YogVal varkwarg)
{
    SAVE_ARGS5(env, callee, self, blockarg, vararg, varkwarg);

    YogVal retval = YogNativeFunction_call_for_instance(env, callee, self, posargc, posargs, blockarg, kwargc, kwargs, vararg, varkwarg);
    YogScriptFrame_push_stack(env, PTR_AS(YogScriptFrame, PTR_AS(YogThread, env->thread)->cur_frame), retval);

    RETURN_VOID(env);
}

static void
YogNativeFunction_exec(YogEnv* env, YogVal callee, uint8_t posargc, YogVal posargs[], YogVal blockarg, uint8_t kwargc, YogVal kwargs[], YogVal vararg, YogVal varkwarg)
{
    YogNativeFunction_exec_for_instance(env, callee, YUNDEF, posargc, posargs, blockarg, kwargc, kwargs, vararg, varkwarg);
}

static void
YogNativeFunction_initialize(YogEnv* env, YogVal self, YogVal klass)
{
    YogBasicObj_init(env, self, 0, klass);

    PTR_AS(YogNativeFunction, self)->func_name = INVALID_ID;
    PTR_AS(YogNativeFunction, self)->f = NULL;
}

static void
YogNativeFunction_keep_children(YogEnv* env, void* ptr, ObjectKeeper keeper, void* heap)
{
    YogBasicObj_keep_children(env, ptr, keeper, heap);
}

static YogVal
YogNativeFunction_allocate(YogEnv* env, YogVal klass)
{
    SAVE_ARG(env, klass);
    YogVal func = YUNDEF;
    PUSH_LOCAL(env, func);

    func = ALLOC_OBJ(env, YogNativeFunction_keep_children, NULL, YogNativeFunction);
    YogNativeFunction_initialize(env, func, klass);

    RETURN(env, func);
}

YogVal
YogNativeFunction_new(YogEnv* env, ID klass_name, const char* func_name, void* f)
{
    ID func_id = YogVM_intern(env, env->vm, func_name);

    YogVal func = YogNativeFunction_allocate(env, env->vm->cNativeFunction);
    PTR_AS(YogNativeFunction, func)->klass_name = klass_name;
    PTR_AS(YogNativeFunction, func)->func_name = func_id;
    PTR_AS(YogNativeFunction, func)->f = f;

    return func;
}

static void
YogInstanceMethod_keep_children(YogEnv* env, void* ptr, ObjectKeeper keeper, void* heap)
{
    YogBasicObj_keep_children(env, ptr, keeper, heap);

    YogInstanceMethod* method = ptr;
#define KEEP(member)    YogGC_keep(env, &method->member, keeper, heap)
    KEEP(self);
    KEEP(f);
#undef KEEP
}

static void
YogInstanceMethod_exec(YogEnv* env, YogVal callee, uint8_t posargc, YogVal posargs[], YogVal blockarg, uint8_t kwargc, YogVal kwargs[], YogVal vararg, YogVal varkwarg)
{
    SAVE_ARGS4(env, callee, blockarg, vararg, varkwarg);
    YogVal self = YUNDEF;
    YogVal f = YUNDEF;
    PUSH_LOCALS2(env, self, f);

    self = PTR_AS(YogInstanceMethod, callee)->self;
    f = PTR_AS(YogInstanceMethod, callee)->f;
    YogFunction_exec_for_instance(env, f, self, posargc, posargs, blockarg, kwargc, kwargs, vararg, varkwarg);

    RETURN_VOID(env);
}

static YogVal
YogInstanceMethod_call(YogEnv* env, YogVal callee, uint8_t posargc, YogVal posargs[], YogVal blockarg, uint8_t kwargc, YogVal kwargs[], YogVal vararg, YogVal varkwarg)
{
    SAVE_ARGS4(env, callee, blockarg, vararg, varkwarg);
    YogVal self = YUNDEF;
    YogVal f = YUNDEF;
    YogVal retval = YUNDEF;
    PUSH_LOCALS3(env, self, f, retval);

    self = PTR_AS(YogInstanceMethod, callee)->self;
    f = PTR_AS(YogInstanceMethod, callee)->f;
    retval = YogFunction_call_for_instance(env, f, self, posargc, posargs, blockarg, kwargc, kwargs, vararg, varkwarg);

    RETURN(env, retval);
}

static void
YogInstanceMethod_initialize(YogEnv* env, YogVal self, YogVal klass)
{
    YogBasicObj_init(env, self, 0, klass);

    PTR_AS(YogInstanceMethod, self)->self = YUNDEF;
    PTR_AS(YogInstanceMethod, self)->f = YUNDEF;
}

static YogVal
create_instance_method(YogEnv* env, YogVal klass)
{
    SAVE_ARG(env, klass);
    YogVal method = YUNDEF;
    PUSH_LOCAL(env, method);

    method = ALLOC_OBJ(env, YogInstanceMethod_keep_children, NULL, YogInstanceMethod);
    YogInstanceMethod_initialize(env, method, klass);

    RETURN(env, method);
}

YogVal
YogInstanceMethod_klass_new(YogEnv* env)
{
    SAVE_LOCALS(env);

    YogVal klass = YUNDEF;
    PUSH_LOCAL(env, klass);

    klass = YogKlass_new(env, "InstanceMethod", env->vm->cObject);
    YogKlass_define_allocator(env, klass, create_instance_method);
    YogKlass_define_caller(env, klass, YogInstanceMethod_call);
    YogKlass_define_executor(env, klass, YogInstanceMethod_exec);

    RETURN(env, klass);
}

YogVal
YogInstanceMethod_new(YogEnv* env)
{
    return create_instance_method(env, env->vm->cInstanceMethod);
}

static void
YogNativeInstanceMethod_exec(YogEnv* env, YogVal callee, uint8_t posargc, YogVal posargs[], YogVal blockarg, uint8_t kwargc, YogVal kwargs[], YogVal vararg, YogVal varkwarg)
{
    SAVE_ARGS4(env, callee, blockarg, vararg, varkwarg);
    YogVal self = YUNDEF;
    YogVal f = YUNDEF;
    PUSH_LOCALS2(env, self, f);

    self = PTR_AS(YogInstanceMethod, callee)->self;
    f = PTR_AS(YogInstanceMethod, callee)->f;
    YogNativeFunction_exec_for_instance(env, f, self, posargc, posargs, blockarg, kwargc, kwargs, vararg, varkwarg);

    RETURN_VOID(env);
}

static YogVal
YogNativeInstanceMethod_call(YogEnv* env, YogVal callee, uint8_t posargc, YogVal posargs[], YogVal blockarg, uint8_t kwargc, YogVal kwargs[], YogVal vararg, YogVal varkwarg)
{
    SAVE_ARGS4(env, callee, blockarg, vararg, varkwarg);
    YogVal self = YUNDEF;
    YogVal f = YUNDEF;
    YogVal retval = YUNDEF;
    PUSH_LOCALS3(env, self, f, retval);

    self = PTR_AS(YogInstanceMethod, callee)->self;
    f = PTR_AS(YogInstanceMethod, callee)->f;
    retval = YogNativeFunction_call_for_instance(env, f, self, posargc, posargs, blockarg, kwargc, kwargs, vararg, varkwarg);

    RETURN(env, retval);
}

YogVal
YogNativeInstanceMethod_new(YogEnv* env)
{
    return create_instance_method(env, env->vm->cNativeInstanceMethod);
}

static YogVal
YogNativeFunction_call_get_descr(YogEnv* env, YogVal self, YogVal obj, YogVal klass)
{
    SAVE_ARGS3(env, self, obj, klass);
    YogVal val = YUNDEF;
    PUSH_LOCAL(env, val);

    val = YogNativeInstanceMethod_new(env);
    PTR_AS(YogInstanceMethod, val)->self = obj;
    PTR_AS(YogInstanceMethod, val)->f = self;

    RETURN(env, val);
}

static void
YogNativeFunction_exec_get_descr(YogEnv* env, YogVal self, YogVal obj, YogVal klass)
{
    SAVE_ARGS3(env, self, obj, klass);
    YogVal retval = YUNDEF;
    PUSH_LOCAL(env, retval);

    retval = YogNativeFunction_call_get_descr(env, self, obj, klass);
    FRAME_PUSH(env, retval);

    RETURN_VOID(env);
}

YogVal
YogNativeFunction_klass_new(YogEnv* env)
{
    SAVE_LOCALS(env);
    YogVal klass = YUNDEF;
    PUSH_LOCAL(env, klass);

    klass = YogKlass_new(env, "NativeFunction", env->vm->cObject);
    YogKlass_define_allocator(env, klass, YogNativeFunction_allocate);
    YogKlass_define_descr_get_executor(env, klass, YogNativeFunction_exec_get_descr);
    YogKlass_define_descr_get_caller(env, klass, YogNativeFunction_call_get_descr);
    YogKlass_define_caller(env, klass, YogNativeFunction_call);
    YogKlass_define_executor(env, klass, YogNativeFunction_exec);

    RETURN(env, klass);
}

YogVal
YogCallable_call(YogEnv* env, YogVal self, uint_t argc, YogVal* args)
{
    SAVE_ARG(env, self);
    YogVal klass = YUNDEF;
    YogVal retval = YUNDEF;
    PUSH_LOCALS2(env, klass, retval);

    klass = YogVal_get_klass(env, self);
    Caller call = PTR_AS(YogKlass, klass)->call;
    YOG_ASSERT(env, call != NULL, "uncallable");

    retval = call(env, self, argc, args, YNIL, 0, NULL, YNIL, YNIL);

    RETURN(env, retval);
}

YogVal
YogCallable_call2(YogEnv* env, YogVal self, uint_t argc, YogVal* args, YogVal block)
{
    SAVE_ARGS2(env, self, block);
    YogVal klass = YUNDEF;
    YogVal retval = YUNDEF;
    PUSH_LOCALS2(env, klass, retval);

    klass = YogVal_get_klass(env, self);
    Caller call = PTR_AS(YogKlass, klass)->call;
    YOG_ASSERT(env, call != NULL, "uncallable");

    retval = call(env, self, argc, args, block, 0, NULL, YNIL, YNIL);

    RETURN(env, retval);
}

YogVal
YogNativeInstanceMethod_klass_new(YogEnv* env)
{
    SAVE_LOCALS(env);
    YogVal klass = YUNDEF;
    PUSH_LOCAL(env, klass);

    klass = YogKlass_new(env, "NativeInstanceMethod", env->vm->cObject);
    YogKlass_define_allocator(env, klass, create_instance_method);
    YogKlass_define_caller(env, klass, YogNativeInstanceMethod_call);
    YogKlass_define_executor(env, klass, YogNativeInstanceMethod_exec);

    RETURN(env, klass);
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
