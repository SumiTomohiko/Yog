#include "yog/arg.h"
#include "yog/array.h"
#include "yog/class.h"
#include "yog/code.h"
#include "yog/dict.h"
#include "yog/error.h"
#include "yog/eval.h"
#include "yog/frame.h"
#include "yog/function.h"
#include "yog/string.h"
#include "yog/vm.h"
#include "yog/yog.h"

typedef YogVal (*Body)(YogEnv*, YogVal, YogVal, YogVal, YogVal, YogVal);

static void
fill_args(YogEnv* env, YogVal arg_info, uint8_t posargc, YogVal posargs[], YogVal blockarg, uint8_t kwargc, YogVal kwargs[], YogVal vararg, YogVal varkwarg, uint_t argc, YogVal args, uint_t args_offset)
{
    SAVE_ARGS5(env, arg_info, blockarg, vararg, varkwarg, args);
    YogVal array = YUNDEF;
    YogVal kw = YUNDEF;
    YogVal va = YUNDEF;
    YogVal iter = YUNDEF;
    PUSH_LOCALS4(env, array, kw, va, iter);

    uint_t i;
    uint_t arg_argc = PTR_AS(YogArgInfo, arg_info)->argc;
    uint_t arg_blockargc = PTR_AS(YogArgInfo, arg_info)->blockargc;
    uint_t size = arg_argc + arg_blockargc;
    for (i = 0; i < size; i++) {
        PTR_AS(YogValArray, args)->items[args_offset + i] = YUNDEF;
    }
    uint_t arg_kwargc = PTR_AS(YogArgInfo, arg_info)->kwargc;
    if (0 < arg_kwargc) {
        uint_t argc = PTR_AS(YogArgInfo, arg_info)->argc;
        uint_t index = argc;
        uint_t blockargc = PTR_AS(YogArgInfo, arg_info)->blockargc;
        if (0 < blockargc) {
            index++;
        }
        uint_t varargc = PTR_AS(YogArgInfo, arg_info)->varargc;
        if (0 < varargc) {
            index++;
        }
        kw = YogDict_new(env);
        PTR_AS(YogValArray, args)->items[args_offset + index] = kw;
    }

    if (arg_argc < posargc) {
        for (i = 0; i < PTR_AS(YogArgInfo, arg_info)->argc; i++) {
            YogVal* items = PTR_AS(YogValArray, args)->items;
            items[args_offset + i] = posargs[i];
        }
        YOG_ASSERT(env, PTR_AS(YogArgInfo, arg_info)->varargc == 1, "Too many arguments.");
        uint_t argc = PTR_AS(YogArgInfo, arg_info)->argc;
        uint_t blockargc = PTR_AS(YogArgInfo, arg_info)->blockargc;
        uint_t index = argc + blockargc;
        array = YogArray_new(env);
        PTR_AS(YogValArray, args)->items[args_offset + index] = array;
        for (i = PTR_AS(YogArgInfo, arg_info)->argc; i < posargc; i++) {
            YogArray_push(env, array, posargs[i]);
        }
        if (IS_PTR(vararg) && (BASIC_OBJ_TYPE(vararg) == TYPE_ARRAY)) {
            YogArray_extend(env, array, vararg);
        }
    }
    else {
        for (i = 0; i < posargc; i++) {
            YogVal* items = PTR_AS(YogValArray, args)->items;
            items[args_offset + i] = posargs[i];
        }
        if (IS_PTR(vararg) && (BASIC_OBJ_TYPE(vararg) == TYPE_ARRAY)) {
            YogVal* items = PTR_AS(YogValArray, args)->items;
            for (i = posargc; i < arg_argc; i++) {
                YogVal val = YogArray_at(env, vararg, i - posargc);
                items[args_offset + i] = val;
            }
        }
        if (0 < PTR_AS(YogArgInfo, arg_info)->varargc) {
            va = YogArray_new(env);
            YogVal* items = PTR_AS(YogValArray, args)->items;
            items[args_offset + posargc] = va;

            if (IS_PTR(vararg) && (BASIC_OBJ_TYPE(vararg) == TYPE_ARRAY)) {
                uint_t size = YogArray_size(env, vararg);
                for (i = arg_argc - posargc; i < size; i++) {
                    YogVal val = YogArray_at(env, vararg, i);
                    YogArray_push(env, va, val);
                }
            }
        }
    }

    for (i = 0; i < kwargc; i++) {
        YogVal name = kwargs[2 * i];
        ID id = VAL2ID(name);
        uint_t j;
        for (j = 0; j < PTR_AS(YogArgInfo, arg_info)->argc; j++) {
            YogVal argnames = PTR_AS(YogArgInfo, arg_info)->argnames;
            ID argname = PTR_AS(ID, argnames)[j];
            if (argname == id) {
                YogVal* items = PTR_AS(YogValArray, args)->items;
                YOG_ASSERT(env, IS_UNDEF(items[args_offset + j]), "Argument specified twice.");
                YogVal val = kwargs[2 * i + 1];
                items[args_offset + j] = val;
                break;
            }
        }
        if (j != PTR_AS(YogArgInfo, arg_info)->argc) {
            continue;
        }
        if (!IS_PTR(kw)) {
            YogVal name = YogVM_id2name(env, env->vm, id);
            YogError_raise_TypeError(env, "an unexpected keyword argument '%s'", STRING_CSTR(name));
        }
        YogDict_set(env, kw, name, kwargs[2 * i + 1]);
    }

    if (IS_PTR(varkwarg) && (BASIC_OBJ_TYPE(varkwarg) == TYPE_DICT)) {
        iter = YogDict_get_iterator(env, varkwarg);
        while (YogDictIterator_next(env, iter)) {
            YogVal key = YogDictIterator_current_key(env, iter);
            YOG_ASSERT(env, IS_SYMBOL(key), "invalid key");
            YogVal value = YogDictIterator_current_value(env, iter);

            ID name = VAL2ID(key);
            uint_t i;
            for (i = 0; i < PTR_AS(YogArgInfo, arg_info)->argc; i++) {
                YogVal argnames = PTR_AS(YogArgInfo, arg_info)->argnames;
                ID argname = PTR_AS(ID, argnames)[i];
                if (argname == name) {
                    YogVal* items = PTR_AS(YogValArray, args)->items;
                    YOG_ASSERT(env, IS_UNDEF(items[args_offset + i]), "argument specified twice.");
                    items[args_offset + i] = value;
                    break;
                }
            }
            if (i != PTR_AS(YogArgInfo, arg_info)->argc) {
                continue;
            }
            YOG_ASSERT(env, IS_PTR(kw), "no keyword parameter");
            YogDict_set(env, kw, key, value);
        }
    }

    if (IS_PTR(blockarg)) {
        YOG_ASSERT(env, PTR_AS(YogArgInfo, arg_info)->blockargc == 1, "Can't accept block argument.");
        YogVal* items = PTR_AS(YogValArray, args)->items;
        uint_t index = PTR_AS(YogArgInfo, arg_info)->argc;
        items[args_offset + index] = blockarg;
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

    YogFunction* f = PTR_AS(YogFunction, ptr);
#define KEEP(member)    YogGC_keep(env, &f->member, keeper, heap)
    KEEP(code);
    KEEP(globals);
    KEEP(outer_vars);
    KEEP(frame_to_long_return);
    KEEP(frame_to_long_break);
    KEEP(klass);
#undef KEEP
}

static void
YogFunction_exec_for_instance(YogEnv* env, YogVal callee, YogVal self, uint8_t posargc, YogVal posargs[], uint8_t kwargc, YogVal kwargs[], YogVal vararg, YogVal varkwarg, YogVal blockarg)
{
    SAVE_ARGS5(env, callee, self, vararg, varkwarg, blockarg);
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
    PTR_AS(YogMethodFrame, frame)->klass = PTR_AS(YogFunction, callee)->klass;
    PTR_AS(YogMethodFrame, frame)->name = PTR_AS(YogFunction, callee)->name;
    PTR_AS(YogScriptFrame, frame)->globals = PTR_AS(YogFunction, callee)->globals;
    PTR_AS(YogScriptFrame, frame)->outer_vars = outer_vars;
    PTR_AS(YogScriptFrame, frame)->frame_to_long_return = PTR_AS(YogFunction, callee)->frame_to_long_return;
    PTR_AS(YogScriptFrame, frame)->frame_to_long_break = PTR_AS(YogFunction, callee)->frame_to_long_break;

    PTR_AS(YogFrame, frame)->prev = env->frame;
    env->frame = frame;

    RETURN_VOID(env);
}

static void
YogFunction_exec(YogEnv* env, YogVal callee, uint8_t posargc, YogVal posargs[], uint8_t kwargc, YogVal kwargs[], YogVal vararg, YogVal varkwarg, YogVal blockarg)
{
    YogFunction_exec_for_instance(env, callee, YUNDEF, posargc, posargs, kwargc, kwargs, vararg, varkwarg, blockarg);
}

static YogVal
YogFunction_call_for_instance(YogEnv* env, YogVal callee, YogVal self, uint8_t posargc, YogVal posargs[], uint8_t kwargc, YogVal kwargs[], YogVal vararg, YogVal varkwarg, YogVal blockarg)
{
    SAVE_ARGS5(env, callee, self, vararg, varkwarg, blockarg);
    YogVal retval = YUNDEF;
    PUSH_LOCAL(env, retval);

    YogEval_push_finish_frame(env);

    YogFunction_exec_for_instance(env, callee, self, posargc, posargs, kwargc, kwargs, vararg, varkwarg, blockarg);
    retval = YogEval_mainloop(env);

    RETURN(env, retval);
}

static YogVal
YogFunction_call(YogEnv* env, YogVal callee, uint8_t posargc, YogVal posargs[], uint8_t kwargc, YogVal kwargs[], YogVal vararg, YogVal varkwarg, YogVal blockarg)
{
    return YogFunction_call_for_instance(env, callee, YUNDEF, posargc, posargs, kwargc, kwargs, vararg, varkwarg, blockarg);
}

static void
YogFunction_init(YogEnv* env, YogVal self, YogVal klass)
{
    YogBasicObj_init(env, self, TYPE_FUNCTION, 0, klass);

    PTR_AS(YogFunction, self)->code = YUNDEF;
    PTR_AS(YogFunction, self)->globals = YUNDEF;
    PTR_AS(YogFunction, self)->outer_vars = YUNDEF;
    PTR_AS(YogFunction, self)->frame_to_long_return = YUNDEF;
    PTR_AS(YogFunction, self)->frame_to_long_break = YUNDEF;
    PTR_AS(YogFunction, self)->klass = YUNDEF;
}

static YogVal
YogFunction_allocate(YogEnv* env, YogVal klass)
{
    SAVE_ARG(env, klass);
    YogVal f = YUNDEF;
    PUSH_LOCAL(env, f);

    f = ALLOC_OBJ(env, YogFunction_keep_children, NULL, YogFunction);
    YogFunction_init(env, f, klass);

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

static YogVal
descr_get(YogEnv* env, YogVal self, YogVal pkg, YogVal args, YogVal kw, YogVal block)
{
    SAVE_ARGS5(env, self, pkg, args, kw, block);
    YogVal obj = YUNDEF;
    YogVal self_class = YUNDEF;
    YogVal retval = YUNDEF;
    YogVal klass = YUNDEF;
    PUSH_LOCALS4(env, obj, self_class, retval, klass);

    obj = YogArray_at(env, args, 0);
    klass = YogVal_get_class(env, obj);

    self_class = YogVal_get_class(env, self);
    YogVal (*f)(YogEnv*, YogVal, YogVal, YogVal) = PTR_AS(YogClass, self_class)->call_get_descr;
    retval = f(env, self, obj, klass);

    RETURN(env, retval);
}

YogVal
YogFunction_define_class(YogEnv* env, YogVal pkg)
{
    SAVE_ARG(env, pkg);
    YogVal klass = YUNDEF;
    PUSH_LOCAL(env, klass);

    klass = YogClass_new(env, "Function", env->vm->cObject);
    YogClass_define_allocator(env, klass, YogFunction_allocate);
    YogClass_define_descr_get_executor(env, klass, YogFunction_exec_get_descr);
    YogClass_define_descr_get_caller(env, klass, YogFunction_call_get_descr);
    YogClass_define_caller(env, klass, YogFunction_call);
    YogClass_define_executor(env, klass, YogFunction_exec);
#define DEFINE_METHOD(name, f)  do { \
    YogClass_define_method(env, klass, pkg, (name), (f)); \
} while (0)
    DEFINE_METHOD("descr_get", descr_get);
#undef DEFINE_METHOD

    RETURN(env, klass);
}

YogVal
YogFunction_new(YogEnv* env)
{
    return YogFunction_allocate(env, env->vm->cFunction);
}

static YogVal
create_keyword_argument(YogEnv* env, uint8_t kwargc, YogVal kwargs[], YogVal varkwarg)
{
    SAVE_ARG(env, varkwarg);
    YogVal kw = YUNDEF;
    YogVal key = YUNDEF;
    YogVal value = YUNDEF;
    PUSH_LOCALS3(env, kw, key, value);

    if ((kwargc == 0) && !IS_PTR(varkwarg)) {
        RETURN(env, YUNDEF);
    }

    kw = YogDict_new(env);
    uint_t i;
    for (i = 0; i < kwargc; i += 2) {
        key = kwargs[i];
        value = kwargs[i + 1];
        YogDict_set(env, kw, key, value);
    }
    if (IS_PTR(varkwarg)) {
        YogDict_add(env, kw, varkwarg);
    }

    RETURN(env, kw);
}

static YogVal
create_positional_argument(YogEnv* env, uint8_t posargc, YogVal posargs[], YogVal vararg)
{
    SAVE_ARG(env, vararg);
    YogVal args = YUNDEF;
    PUSH_LOCAL(env, args);

    args = YogArray_new(env);
    uint_t i;
    for (i = 0; i < posargc; i++) {
        YogArray_push(env, args, posargs[i]);
    }

    if (IS_PTR(vararg) && (BASIC_OBJ_TYPE(vararg) == TYPE_ARRAY)) {
        YogArray_add(env, args, vararg);
    }

    RETURN(env, args);
}

static YogVal
YogNativeFunction_call_for_instance(YogEnv* env, YogVal callee, YogVal self, uint8_t posargc, YogVal posargs[], uint8_t kwargc, YogVal kwargs[], YogVal vararg, YogVal varkwarg, YogVal blockarg)
{
    SAVE_ARGS5(env, callee, self, vararg, varkwarg, blockarg);
    YogVal args = YUNDEF;
    YogVal retval = YUNDEF;
    YogVal frame = YUNDEF;
    YogVal kw = YUNDEF;
    YogVal pkg = YUNDEF;
    PUSH_LOCALS5(env, args, retval, frame, kw, pkg);

    args = create_positional_argument(env, posargc, posargs, vararg);
    kw = create_keyword_argument(env, kwargc, kwargs, varkwarg);

    frame = YogCFrame_new(env);
    PTR_AS(YogCFrame, frame)->f = callee;
    PTR_AS(YogFrame, frame)->prev = env->frame;
    env->frame = frame;

    Body f = (Body)PTR_AS(YogNativeFunction, callee)->f;
    pkg = PTR_AS(YogNativeFunction, callee)->pkg;
    retval = (*f)(env, self, pkg, args, kw, blockarg);

    env->frame = PTR_AS(YogFrame, env->frame)->prev;

    RETURN(env, retval);
}

static YogVal
YogNativeFunction_call(YogEnv* env, YogVal callee, uint8_t posargc, YogVal posargs[], uint8_t kwargc, YogVal kwargs[], YogVal vararg, YogVal varkwarg, YogVal blockarg)
{
    return YogNativeFunction_call_for_instance(env, callee, YUNDEF, posargc, posargs, kwargc, kwargs, vararg, varkwarg, blockarg);;
}

static void
YogNativeFunction_exec_for_instance(YogEnv* env, YogVal callee, YogVal self, uint8_t posargc, YogVal posargs[], uint8_t kwargc, YogVal kwargs[], YogVal vararg, YogVal varkwarg, YogVal blockarg)
{
    SAVE_ARGS5(env, callee, self, vararg, varkwarg, blockarg);
    YogVal retval = YUNDEF;
    YogVal frame = YUNDEF;
    PUSH_LOCALS2(env, retval, frame);

    retval = YogNativeFunction_call_for_instance(env, callee, self, posargc, posargs, kwargc, kwargs, vararg, varkwarg, blockarg);
    YogEval_push_returned_value(env, env->frame, retval);

    RETURN_VOID(env);
}

static void
YogNativeFunction_exec(YogEnv* env, YogVal callee, uint8_t posargc, YogVal posargs[], uint8_t kwargc, YogVal kwargs[], YogVal vararg, YogVal varkwarg, YogVal blockarg)
{
    YogNativeFunction_exec_for_instance(env, callee, YUNDEF, posargc, posargs, kwargc, kwargs, vararg, varkwarg, blockarg);
}

static void
YogNativeFunction_init(YogEnv* env, YogVal self, YogVal klass)
{
    YogBasicObj_init(env, self, TYPE_NATIVE_FUNCTION, 0, klass);

    PTR_AS(YogNativeFunction, self)->class_name = INVALID_ID;
    PTR_AS(YogNativeFunction, self)->pkg = YUNDEF;
    PTR_AS(YogNativeFunction, self)->func_name = INVALID_ID;
    PTR_AS(YogNativeFunction, self)->f = NULL;
}

static void
YogNativeFunction_keep_children(YogEnv* env, void* ptr, ObjectKeeper keeper, void* heap)
{
    YogBasicObj_keep_children(env, ptr, keeper, heap);

    YogNativeFunction* f = (YogNativeFunction*)ptr;
#define KEEP(member)    YogGC_keep(env, &f->member, keeper, heap)
    KEEP(pkg);
#undef KEEP
}

static YogVal
YogNativeFunction_allocate(YogEnv* env, YogVal klass)
{
    SAVE_ARG(env, klass);
    YogVal func = YUNDEF;
    PUSH_LOCAL(env, func);

    func = ALLOC_OBJ(env, YogNativeFunction_keep_children, NULL, YogNativeFunction);
    YogNativeFunction_init(env, func, klass);

    RETURN(env, func);
}

YogVal
YogNativeFunction_new(YogEnv* env, ID class_name, YogVal pkg, const char* func_name, void* f)
{
    SAVE_ARG(env, pkg);
    YogVal func = YUNDEF;
    PUSH_LOCAL(env, func);

    ID func_id = YogVM_intern(env, env->vm, func_name);

    func = YogNativeFunction_allocate(env, env->vm->cNativeFunction);
    PTR_AS(YogNativeFunction, func)->class_name = class_name;
    PTR_AS(YogNativeFunction, func)->pkg = pkg;
    PTR_AS(YogNativeFunction, func)->func_name = func_id;
    PTR_AS(YogNativeFunction, func)->f = (Body)f;

    RETURN(env, func);
}

static void
YogInstanceMethod_keep_children(YogEnv* env, void* ptr, ObjectKeeper keeper, void* heap)
{
    YogBasicObj_keep_children(env, ptr, keeper, heap);

    YogInstanceMethod* method = PTR_AS(YogInstanceMethod, ptr);
#define KEEP(member)    YogGC_keep(env, &method->member, keeper, heap)
    KEEP(self);
    KEEP(f);
#undef KEEP
}

static void
YogInstanceMethod_exec(YogEnv* env, YogVal callee, uint8_t posargc, YogVal posargs[], uint8_t kwargc, YogVal kwargs[], YogVal vararg, YogVal varkwarg, YogVal blockarg)
{
    SAVE_ARGS4(env, callee, vararg, varkwarg, blockarg);
    YogVal self = YUNDEF;
    YogVal f = YUNDEF;
    PUSH_LOCALS2(env, self, f);

    self = PTR_AS(YogInstanceMethod, callee)->self;
    f = PTR_AS(YogInstanceMethod, callee)->f;
    YogFunction_exec_for_instance(env, f, self, posargc, posargs, kwargc, kwargs, vararg, varkwarg, blockarg);

    RETURN_VOID(env);
}

static YogVal
YogInstanceMethod_call(YogEnv* env, YogVal callee, uint8_t posargc, YogVal posargs[], uint8_t kwargc, YogVal kwargs[], YogVal vararg, YogVal varkwarg, YogVal blockarg)
{
    SAVE_ARGS4(env, callee, vararg, varkwarg, blockarg);
    YogVal self = YUNDEF;
    YogVal f = YUNDEF;
    YogVal retval = YUNDEF;
    PUSH_LOCALS3(env, self, f, retval);

    self = PTR_AS(YogInstanceMethod, callee)->self;
    f = PTR_AS(YogInstanceMethod, callee)->f;
    retval = YogFunction_call_for_instance(env, f, self, posargc, posargs, kwargc, kwargs, vararg, varkwarg, blockarg);

    RETURN(env, retval);
}

static void
YogInstanceMethod_init(YogEnv* env, YogVal self, YogVal klass)
{
    YogBasicObj_init(env, self, TYPE_INSTANCE_METHOD, 0, klass);

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
    YogInstanceMethod_init(env, method, klass);

    RETURN(env, method);
}

YogVal
YogInstanceMethod_define_class(YogEnv* env, YogVal pkg)
{
    SAVE_ARG(env, pkg);
    YogVal klass = YUNDEF;
    PUSH_LOCAL(env, klass);

    klass = YogClass_new(env, "InstanceMethod", env->vm->cObject);
    YogClass_define_allocator(env, klass, create_instance_method);
    YogClass_define_caller(env, klass, YogInstanceMethod_call);
    YogClass_define_executor(env, klass, YogInstanceMethod_exec);

    RETURN(env, klass);
}

YogVal
YogInstanceMethod_new(YogEnv* env)
{
    return create_instance_method(env, env->vm->cInstanceMethod);
}

static void
YogNativeInstanceMethod_exec(YogEnv* env, YogVal callee, uint8_t posargc, YogVal posargs[], uint8_t kwargc, YogVal kwargs[], YogVal vararg, YogVal varkwarg, YogVal blockarg)
{
    SAVE_ARGS4(env, callee, vararg, varkwarg, blockarg);
    YogVal self = YUNDEF;
    YogVal f = YUNDEF;
    PUSH_LOCALS2(env, self, f);

    self = PTR_AS(YogInstanceMethod, callee)->self;
    f = PTR_AS(YogInstanceMethod, callee)->f;
    YogNativeFunction_exec_for_instance(env, f, self, posargc, posargs, kwargc, kwargs, vararg, varkwarg, blockarg);

    RETURN_VOID(env);
}

static YogVal
YogNativeInstanceMethod_call(YogEnv* env, YogVal callee, uint8_t posargc, YogVal posargs[], uint8_t kwargc, YogVal kwargs[], YogVal vararg, YogVal varkwarg, YogVal blockarg)
{
    SAVE_ARGS4(env, callee, vararg, varkwarg, blockarg);
    YogVal self = YUNDEF;
    YogVal f = YUNDEF;
    YogVal retval = YUNDEF;
    PUSH_LOCALS3(env, self, f, retval);

    self = PTR_AS(YogInstanceMethod, callee)->self;
    f = PTR_AS(YogInstanceMethod, callee)->f;
    retval = YogNativeFunction_call_for_instance(env, f, self, posargc, posargs, kwargc, kwargs, vararg, varkwarg, blockarg);

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
YogNativeFunction_define_class(YogEnv* env, YogVal pkg)
{
    SAVE_ARG(env, pkg);
    YogVal klass = YUNDEF;
    PUSH_LOCAL(env, klass);

    klass = YogClass_new(env, "NativeFunction", env->vm->cObject);
    YogClass_define_allocator(env, klass, YogNativeFunction_allocate);
    YogClass_define_descr_get_executor(env, klass, YogNativeFunction_exec_get_descr);
    YogClass_define_descr_get_caller(env, klass, YogNativeFunction_call_get_descr);
    YogClass_define_caller(env, klass, YogNativeFunction_call);
    YogClass_define_executor(env, klass, YogNativeFunction_exec);
#define DEFINE_METHOD(name, f)  do { \
    YogClass_define_method(env, klass, pkg, (name), (f)); \
} while (0)
    DEFINE_METHOD("descr_get", descr_get);
#undef DEFINE_METHOD

    RETURN(env, klass);
}

YogVal
YogCallable_call(YogEnv* env, YogVal self, uint_t argc, YogVal* args)
{
    SAVE_ARG(env, self);
    YogVal klass = YUNDEF;
    YogVal retval = YUNDEF;
    PUSH_LOCALS2(env, klass, retval);

    klass = YogVal_get_class(env, self);
    Caller call = PTR_AS(YogClass, klass)->call;
    YOG_ASSERT(env, call != NULL, "uncallable");

    retval = call(env, self, argc, args, 0, NULL, YNIL, YNIL, YNIL);

    RETURN(env, retval);
}

YogVal
YogCallable_call1(YogEnv* env, YogVal self, YogVal arg0)
{
    SAVE_ARGS2(env, self, arg0);
    YogVal retval = YUNDEF;
    PUSH_LOCAL(env, retval);
    YogVal args[] = { arg0 };
    PUSH_LOCALSX(env, array_sizeof(args), args);

    retval = YogCallable_call(env, self, array_sizeof(args), args);

    RETURN(env, retval);
}

YogVal
YogCallable_call2(YogEnv* env, YogVal self, uint_t argc, YogVal* args, YogVal block)
{
    SAVE_ARGS2(env, self, block);
    YogVal klass = YUNDEF;
    YogVal retval = YUNDEF;
    PUSH_LOCALS2(env, klass, retval);

    klass = YogVal_get_class(env, self);
    Caller call = PTR_AS(YogClass, klass)->call;
    YOG_ASSERT(env, call != NULL, "uncallable");

    retval = call(env, self, argc, args, 0, NULL, YNIL, YNIL, block);

    RETURN(env, retval);
}

YogVal
YogNativeInstanceMethod_define_class(YogEnv* env, YogVal pkg)
{
    SAVE_ARG(env, pkg);
    YogVal klass = YUNDEF;
    PUSH_LOCAL(env, klass);

    klass = YogClass_new(env, "NativeInstanceMethod", env->vm->cObject);
    YogClass_define_allocator(env, klass, create_instance_method);
    YogClass_define_caller(env, klass, YogNativeInstanceMethod_call);
    YogClass_define_executor(env, klass, YogNativeInstanceMethod_exec);

    RETURN(env, klass);
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
