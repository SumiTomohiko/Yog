#include "yog/arg.h"
#include "yog/array.h"
#include "yog/callable.h"
#include "yog/class.h"
#include "yog/code.h"
#include "yog/dict.h"
#include "yog/error.h"
#include "yog/eval.h"
#include "yog/frame.h"
#include "yog/string.h"
#include "yog/vm.h"
#include "yog/yog.h"

typedef YogVal (*Body)(YogEnv*, YogVal, YogVal, YogVal, YogVal, YogVal);

static void
raise_TypeError(YogEnv* env, const char* mark, const char* needed, YogVal actual)
{
    SAVE_ARG(env, actual);
    YogError_raise_TypeError(env, "argument after %s must be %s, not %C", mark, needed, actual);
    /* NOTREACHED */
    RETURN_VOID(env);
}

static void
raise_TypeError_for_vararg(YogEnv* env, YogVal actual)
{
    SAVE_ARG(env, actual);
    raise_TypeError(env, "*", "an Array", actual);
    RETURN_VOID(env);
}

static void
raise_TypeError_for_varkwarg(YogEnv* env, YogVal actual)
{
    SAVE_ARG(env, actual);
    raise_TypeError(env, "**", "a Dict", actual);
    RETURN_VOID(env);
}

#define STORE_LOCAL(env, frame, index, val) YogGC_UPDATE_PTR((env), SCRIPT_FRAME((frame)), locals_etc[SCRIPT_FRAME((frame))->stack_capacity + (index)], (val))

static void
assign_keyword_arg(YogEnv* env, YogVal self, uint_t args_offset, YogVal frame, YogVal kw, ID name, YogVal val)
{
    SAVE_ARGS4(env, self, frame, kw, val);
    YogVal names = YUNDEF;
    YogVal code = YUNDEF;
    YogVal formal_args = YUNDEF;
    YogVal v = YUNDEF;
    PUSH_LOCALS4(env, names, code, formal_args, v);

    code = PTR_AS(YogFunction, self)->code;
    formal_args = PTR_AS(YogCode, code)->arg_info;
    uint_t argc = PTR_AS(YogArgInfo, formal_args)->argc;
    uint_t i;
    for (i = 0; i < argc; i++) {
        names = PTR_AS(YogArgInfo, formal_args)->argnames;
        if (PTR_AS(ID, names)[i] != name) {
            continue;
        }
        v = SCRIPT_FRAME_LOCALS(frame)[args_offset + i];
        if (!IS_UNDEF(v)) {
            YogError_raise_ArgumentError(env, "%I() got multiple values for keyword argument \"%I\"", PTR_AS(YogFunction, self)->name, name);
        }
        STORE_LOCAL(env, frame, args_offset + i, v);
        RETURN_VOID(env);
    }
    if (!IS_PTR(kw)) {
        YogError_raise_ArgumentError(env, "an unexpected keyword argument \"%I\"", name);
    }
    YogDict_set(env, kw, ID2VAL(name), val);

    RETURN_VOID(env);
}

static void
raise_wrong_num_args(YogEnv* env, YogVal self, uint_t posargc)
{
    SAVE_ARG(env, self);
    YogVal formal_args = YUNDEF;
    YogVal code = YUNDEF;
    PUSH_LOCALS2(env, formal_args, code);

    code = PTR_AS(YogFunction, self)->code;
    formal_args = PTR_AS(YogCode, code)->arg_info;
    uint_t formal_posargc = PTR_AS(YogArgInfo, formal_args)->argc;
    YogError_raise_ArgumentError(env, "%I() requires %u positional argument(s) (%u given)", PTR_AS(YogFunction, self)->name, formal_posargc, posargc);

    RETURN_VOID(env);
}

static void
check_arg_assigned(YogEnv* env, YogVal self, YogVal arg, uint_t posargc)
{
    SAVE_ARGS2(env, self, arg);
    if (!IS_UNDEF(arg)) {
        RETURN_VOID(env);
    }
    raise_wrong_num_args(env, self, posargc);
    RETURN_VOID(env);
}

static void
fill_args(YogEnv* env, YogVal self, uint_t args_offset, uint8_t posargc, YogVal posargs[], YogVal blockarg, uint8_t kwargc, YogVal kwargs[], YogVal vararg, YogVal varkwarg, YogVal frame)
{
    SAVE_ARGS5(env, self, blockarg, vararg, varkwarg, frame);
    YogVal array = YUNDEF;
    YogVal kw = YUNDEF;
    YogVal va = YUNDEF;
    YogVal iter = YUNDEF;
    YogVal val = YUNDEF;
    YogVal code = PTR_AS(YogFunction, self)->code;
    YogVal arg_info = PTR_AS(YogCode, code)->arg_info;
    PUSH_LOCALS7(env, array, kw, va, iter, val, code, arg_info);
    YOG_ASSERT(env, IS_PTR(self), "invalid self (0x%x)", self);
    YOG_ASSERT(env, BASIC_OBJ_TYPE(self) == TYPE_FUNCTION, "invalid type self (0x%x)", BASIC_OBJ_TYPE(self));
    if (!IS_PTR(arg_info)) {
        RETURN_VOID(env);
    }

    uint_t arg_kwargc = PTR_AS(YogArgInfo, arg_info)->kwargc;
    if (0 < arg_kwargc) {
        uint_t index = PTR_AS(YogArgInfo, arg_info)->argc;
        uint_t varargc = PTR_AS(YogArgInfo, arg_info)->varargc;
        if (0 < varargc) {
            index++;
        }
        kw = YogDict_new(env);
        STORE_LOCAL(env, frame, args_offset + index, kw);
    }

    uint_t i;
    uint_t arg_argc = PTR_AS(YogArgInfo, arg_info)->argc;
    if (arg_argc < posargc) {
        uint_t argc = PTR_AS(YogArgInfo, arg_info)->argc;
        for (i = 0; i < argc; i++) {
            STORE_LOCAL(env, frame, args_offset + i, posargs[i]);
        }
        if (PTR_AS(YogArgInfo, arg_info)->varargc != 1) {
            raise_wrong_num_args(env, self, posargc);
        }
        array = YogArray_new(env);
        STORE_LOCAL(env, frame, args_offset + argc, array);
        for (i = argc; i < posargc; i++) {
            YogArray_push(env, array, posargs[i]);
        }
        if (IS_UNDEF(vararg)) {
            /* Do nothing */
        }
        else if (!IS_PTR(vararg) || (BASIC_OBJ_TYPE(vararg) != TYPE_ARRAY)) {
            raise_TypeError_for_vararg(env, vararg);
        }
        else {
            YogArray_extend(env, array, vararg);
        }
    }
    else {
        for (i = 0; i < posargc; i++) {
            STORE_LOCAL(env, frame, args_offset + i, posargs[i]);
        }
        if (IS_UNDEF(vararg)) {
            /* Do nothing */
        }
        else if (!IS_PTR(vararg) || (BASIC_OBJ_TYPE(vararg) != TYPE_ARRAY)) {
            raise_TypeError_for_vararg(env, vararg);
        }
        else {
            for (i = posargc; i < arg_argc; i++) {
                YogVal val = YogArray_at(env, vararg, i - posargc);
                STORE_LOCAL(env, frame, args_offset + i, val);
            }
        }
        if (0 < PTR_AS(YogArgInfo, arg_info)->varargc) {
            va = YogArray_new(env);
            STORE_LOCAL(env, frame, args_offset + posargc, va);

            if (IS_UNDEF(vararg)) {
                /* Do nothing */
            }
            else if (!IS_PTR(vararg) || (BASIC_OBJ_TYPE(vararg) != TYPE_ARRAY)) {
                raise_TypeError_for_vararg(env, vararg);
            }
            else {
                uint_t size = YogArray_size(env, vararg);
                for (i = arg_argc - posargc; i < size; i++) {
                    YogVal val = YogArray_at(env, vararg, i);
                    YogArray_push(env, va, val);
                }
            }
        }
    }

    for (i = 0; i < kwargc; i++) {
        ID name = VAL2ID(kwargs[2 * i]);
        val = kwargs[2 * i + 1];
        assign_keyword_arg(env, self, args_offset, frame, kw, name, val);
    }

    if (IS_UNDEF(varkwarg)) {
        /* Do nothing */
    }
    else if (!IS_PTR(varkwarg) || (BASIC_OBJ_TYPE(varkwarg) != TYPE_DICT)) {
        raise_TypeError_for_varkwarg(env, varkwarg);
    }
    else {
        iter = YogDict_get_iterator(env, varkwarg);
        while (YogDictIterator_next(env, iter)) {
            YogVal key = YogDictIterator_current_key(env, iter);
            if (!IS_SYMBOL(key)) {
                YogError_raise_TypeError(env, "keywords must be symbols, not %C", key);
            }
            val = YogDictIterator_current_value(env, iter);
            assign_keyword_arg(env, self, args_offset, frame, kw, VAL2ID(key), val);
        }
    }

    if (!IS_UNDEF(blockarg)) {
        if (PTR_AS(YogArgInfo, arg_info)->blockargc != 1) {
            YogError_raise_ArgumentError(env, "can't accept a block argument");
        }
        uint_t index = PTR_AS(YogArgInfo, arg_info)->argc;
        if (0 < PTR_AS(YogArgInfo, arg_info)->varargc) {
            index++;
        }
        if (0 < PTR_AS(YogArgInfo, arg_info)->kwargc) {
            index++;
        }
        STORE_LOCAL(env, frame, args_offset + index, blockarg);
    }

    uint_t i;
    uint_t required_argc = PTR_AS(YogArgInfo, arg_info)->required_argc;
    for (i = 0; i < required_argc; i++) {
        val = PTR_AS(YogValArray, frame)->items[args_offset + i];
        check_arg_assigned(env, self, val, posargc);
    }

    RETURN_VOID(env);
}

static void
YogFunction_keep_children(YogEnv* env, void* ptr, ObjectKeeper keeper, void* heap)
{
    YogBasicObj_keep_children(env, ptr, keeper, heap);

    YogFunction* f = PTR_AS(YogFunction, ptr);
#define KEEP(member)    YogGC_KEEP(env, f, member, keeper, heap)
    KEEP(code);
    KEEP(globals);
    KEEP(outer_frame);
    KEEP(frame_to_long_return);
    KEEP(frame_to_long_break);
#undef KEEP
}

static void
fill_outer_frames(YogEnv* env, YogVal frame, YogVal outer_frame)
{
    SAVE_ARGS2(env, frame, outer_frame);
    if (PTR_AS(YogScriptFrame, frame)->outer_frames_num == 0) {
        RETURN_VOID(env);
    }
    YOG_ASSERT(env, PTR_AS(YogScriptFrame, frame)->outer_frames_num == PTR_AS(YogScriptFrame, outer_frame)->outer_frames_num + 1, "Depth unmatched (%u, %u)", PTR_AS(YogScriptFrame, frame)->outer_frames_num, PTR_AS(YogScriptFrame, outer_frame)->outer_frames_num);

    uint_t offset = PTR_AS(YogScriptFrame, frame)->stack_capacity + PTR_AS(YogScriptFrame, frame)->locals_num;
    YogGC_UPDATE_PTR(env, PTR_AS(YogScriptFrame, frame), locals_etc[offset], outer_frame);
    uint_t depth = PTR_AS(YogScriptFrame, outer_frame)->outer_frames_num;
    uint_t i;
    for (i = 0; i < depth; i++) {
        YogGC_UPDATE_PTR(env, PTR_AS(YogScriptFrame, frame), locals_etc[offset + 1 + i], SCRIPT_FRAME_OUTER_FRAMES(outer_frame)[i]);
    }

    RETURN_VOID(env);
}

static void
YogFunction_exec_for_instance(YogEnv* env, YogVal callee, YogVal self, uint8_t posargc, YogVal posargs[], uint8_t kwargc, YogVal kwargs[], YogVal vararg, YogVal varkwarg, YogVal blockarg)
{
    SAVE_ARGS5(env, callee, self, vararg, varkwarg, blockarg);
    YogVal code = YUNDEF;
    YogVal frame = YUNDEF;
    YogVal frame_to_long_return = YUNDEF;
    PUSH_LOCALS3(env, code, frame, frame_to_long_return);

    code = PTR_AS(YogFunction, callee)->code;
    uint_t locals_num = PTR_AS(YogCode, code)->local_vars_count;
    frame = YogScriptFrame_new(env, FRAME_SCRIPT, code, locals_num, 0);
    uint_t args_offset;
    if (PTR_AS(YogFunction, callee)->needs_self) {
        uint_t pos = PTR_AS(YogScriptFrame, frame)->stack_capacity;
        YogGC_UPDATE_PTR(env, PTR_AS(YogScriptFrame, frame), locals_etc[pos], self);
        args_offset = 1;
    }
    else {
        args_offset = 0;
    }
    fill_args(env, callee, args_offset, posargc, posargs, blockarg, kwargc, kwargs, vararg, varkwarg, frame);

    YogGC_UPDATE_PTR(env, PTR_AS(YogScriptFrame, frame), globals, PTR_AS(YogFunction, callee)->globals);
    fill_outer_frames(env, frame, PTR_AS(YogFunction, callee)->outer_frame);

    frame_to_long_return = PTR_AS(YogFunction, callee)->frame_to_long_return;
    YogGC_UPDATE_PTR(env, PTR_AS(YogScriptFrame, frame), frame_to_long_return, IS_PTR(frame_to_long_return) ? frame_to_long_return : env->frame);
    YogGC_UPDATE_PTR(env, PTR_AS(YogScriptFrame, frame), frame_to_long_break, PTR_AS(YogFunction, callee)->frame_to_long_break);

    if (!IS_UNDEF(self)) {
        YogGC_UPDATE_PTR(env, PTR_AS(YogScriptFrame, frame), klass, YogVal_get_class(env, self));
    }
    PTR_AS(YogScriptFrame, frame)->name = PTR_AS(YogFunction, callee)->name;

    YogGC_UPDATE_PTR(env, PTR_AS(YogFrame, frame), prev, env->frame);
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
    PTR_AS(YogFunction, self)->outer_frame = YUNDEF;
    PTR_AS(YogFunction, self)->frame_to_long_return = YUNDEF;
    PTR_AS(YogFunction, self)->frame_to_long_break = YUNDEF;
    PTR_AS(YogFunction, self)->needs_self = TRUE;
}

static YogVal
YogFunction_alloc(YogEnv* env, YogVal klass)
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
    YogGC_UPDATE_PTR(env, PTR_AS(YogInstanceMethod, val), self, obj);
    YogGC_UPDATE_PTR(env, PTR_AS(YogInstanceMethod, val), f, self);

    RETURN(env, val);
}

static void
YogFunction_exec_get_descr(YogEnv* env, YogVal self, YogVal obj, YogVal klass)
{
    SAVE_ARGS3(env, self, obj, klass);
    YogVal retval = YUNDEF;
    PUSH_LOCAL(env, retval);

    retval = YogFunction_call_get_descr(env, self, obj, klass);
    YogScriptFrame_push_stack(env, env->frame, retval);

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
YogFunction_new(YogEnv* env)
{
    return YogFunction_alloc(env, env->vm->cFunction);
}

static YogVal
create_keyword_argument(YogEnv* env, uint8_t kwargc, YogVal kwargs[], YogVal varkwarg)
{
    SAVE_ARG(env, varkwarg);
    YogVal kw = YUNDEF;
    YogVal key = YUNDEF;
    YogVal value = YUNDEF;
    PUSH_LOCALS3(env, kw, key, value);

    if ((kwargc == 0) && IS_UNDEF(varkwarg)) {
        RETURN(env, YUNDEF);
    }

    kw = YogDict_new(env);
    uint_t i;
    for (i = 0; i < kwargc; i += 2) {
        key = kwargs[i];
        value = kwargs[i + 1];
        YogDict_set(env, kw, key, value);
    }
    if (IS_UNDEF(varkwarg)) {
    }
    else if (!IS_PTR(varkwarg) || (BASIC_OBJ_TYPE(varkwarg) != TYPE_DICT)) {
        raise_TypeError_for_varkwarg(env, varkwarg);
    }
    else {
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

    if (IS_UNDEF(vararg)) {
    }
    else if (!IS_PTR(vararg) || (BASIC_OBJ_TYPE(vararg) != TYPE_ARRAY)) {
        raise_TypeError_for_vararg(env, vararg);
    }
    else {
        YogArray_add(env, args, vararg);
    }

    RETURN(env, args);
}

static YogVal
YogNativeFunction_call_for_instance(YogEnv* env, YogVal callee, YogVal self, uint8_t posargc, YogVal posargs[], uint8_t kwargc, YogVal kwargs[], YogVal vararg, YogVal varkwarg, YogVal blockarg, YogVal* multi_val)
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
    YogGC_UPDATE_PTR(env, PTR_AS(YogCFrame, frame), f, callee);
    YogGC_UPDATE_PTR(env, PTR_AS(YogFrame, frame), prev, env->frame);
    env->frame = frame;

    Body f = (Body)PTR_AS(YogNativeFunction, callee)->f;
    pkg = PTR_AS(YogNativeFunction, callee)->pkg;
    retval = (*f)(env, self, pkg, args, kw, blockarg);
    if (IS_UNDEF(retval) && (multi_val != NULL)) {
        *multi_val = PTR_AS(YogCFrame, frame)->multi_val;
    }

    env->frame = PTR_AS(YogFrame, env->frame)->prev;

    RETURN(env, retval);
}

static void
error_for_multi_value(YogEnv* env, YogVal retval)
{
    SAVE_ARG(env, retval);
    if (!IS_UNDEF(retval)) {
        RETURN_VOID(env);
    }
    YogError_raise_ValueError(env, "multiple value are not allowed");
    RETURN_VOID(env);
}

static YogVal
YogNativeFunction_call(YogEnv* env, YogVal callee, uint8_t posargc, YogVal posargs[], uint8_t kwargc, YogVal kwargs[], YogVal vararg, YogVal varkwarg, YogVal blockarg)
{
    SAVE_ARGS4(env, callee, vararg, varkwarg, blockarg);
    YogVal retval = YUNDEF;
    PUSH_LOCAL(env, retval);

    retval = YogNativeFunction_call_for_instance(env, callee, YUNDEF, posargc, posargs, kwargc, kwargs, vararg, varkwarg, blockarg, NULL);
    error_for_multi_value(env, retval);

    RETURN(env, retval);
}

static void
YogNativeFunction_exec_for_instance(YogEnv* env, YogVal callee, YogVal self, uint8_t posargc, YogVal posargs[], uint8_t kwargc, YogVal kwargs[], YogVal vararg, YogVal varkwarg, YogVal blockarg)
{
    SAVE_ARGS5(env, callee, self, vararg, varkwarg, blockarg);
    YogVal retval = YUNDEF;
    YogVal multi_val = YUNDEF;
    PUSH_LOCALS2(env, retval, multi_val);

    retval = YogNativeFunction_call_for_instance(env, callee, self, posargc, posargs, kwargc, kwargs, vararg, varkwarg, blockarg, &multi_val);
    if (IS_UNDEF(retval)) {
        YogEval_push_returned_multi_value(env, multi_val);
        RETURN_VOID(env);
    }

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
#define KEEP(member)    YogGC_KEEP(env, f, member, keeper, heap)
    KEEP(pkg);
#undef KEEP
}

static YogVal
YogNativeFunction_alloc(YogEnv* env, YogVal klass)
{
    SAVE_ARG(env, klass);
    YogVal func = YUNDEF;
    PUSH_LOCAL(env, func);

    func = ALLOC_OBJ(env, YogNativeFunction_keep_children, NULL, YogNativeFunction);
    YogNativeFunction_init(env, func, klass);

    RETURN(env, func);
}

YogVal
YogNativeFunction_new(YogEnv* env, ID class_name, YogVal pkg, const char* func_name, YogAPI f)
{
    SAVE_ARG(env, pkg);
    YogVal func = YUNDEF;
    PUSH_LOCAL(env, func);

    ID func_id = YogVM_intern(env, env->vm, func_name);

    func = YogNativeFunction_alloc(env, env->vm->cNativeFunction);
    PTR_AS(YogNativeFunction, func)->class_name = class_name;
    YogGC_UPDATE_PTR(env, PTR_AS(YogNativeFunction, func), pkg, pkg);
    PTR_AS(YogNativeFunction, func)->func_name = func_id;
    PTR_AS(YogNativeFunction, func)->f = (Body)f;

    RETURN(env, func);
}

static void
YogInstanceMethod_keep_children(YogEnv* env, void* ptr, ObjectKeeper keeper, void* heap)
{
    YogBasicObj_keep_children(env, ptr, keeper, heap);

    YogInstanceMethod* method = PTR_AS(YogInstanceMethod, ptr);
#define KEEP(member)    YogGC_KEEP(env, method, member, keeper, heap)
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
    retval = YogNativeFunction_call_for_instance(env, f, self, posargc, posargs, kwargc, kwargs, vararg, varkwarg, blockarg, NULL);
    error_for_multi_value(env, retval);

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
    YogGC_UPDATE_PTR(env, PTR_AS(YogInstanceMethod, val), self, obj);
    YogGC_UPDATE_PTR(env, PTR_AS(YogInstanceMethod, val), f, self);

    RETURN(env, val);
}

static void
YogNativeFunction_exec_get_descr(YogEnv* env, YogVal self, YogVal obj, YogVal klass)
{
    SAVE_ARGS3(env, self, obj, klass);
    YogVal retval = YUNDEF;
    PUSH_LOCAL(env, retval);

    retval = YogNativeFunction_call_get_descr(env, self, obj, klass);
    YogScriptFrame_push_stack(env, env->frame, retval);

    RETURN_VOID(env);
}

void
YogFunction_define_classes(YogEnv* env, YogVal pkg)
{
    SAVE_ARG(env, pkg);
    YogVal cNativeFunction = YUNDEF;
    YogVal cFunction = YUNDEF;
    YogVal cInstanceMethod = YUNDEF;
    YogVal cNativeInstanceMethod = YUNDEF;
    PUSH_LOCALS4(env, cNativeFunction, cFunction, cInstanceMethod, cNativeInstanceMethod);
    YogVM* vm = env->vm;

    cNativeFunction = YogClass_new(env, "NativeFunction", vm->cObject);
    YogClass_define_allocator(env, cNativeFunction, YogNativeFunction_alloc);
    YogClass_define_descr_get_executor(env, cNativeFunction, YogNativeFunction_exec_get_descr);
    YogClass_define_descr_get_caller(env, cNativeFunction, YogNativeFunction_call_get_descr);
    YogClass_define_caller(env, cNativeFunction, YogNativeFunction_call);
    YogClass_define_executor(env, cNativeFunction, YogNativeFunction_exec);
#define DEFINE_METHOD(name, f)  do { \
    YogClass_define_method(env, cNativeFunction, pkg, (name), (f)); \
} while (0)
    DEFINE_METHOD("descr_get", descr_get);
#undef DEFINE_METHOD
    vm->cNativeFunction = cNativeFunction;

    cFunction = YogClass_new(env, "Function", vm->cObject);
    YogClass_define_allocator(env, cFunction, YogFunction_alloc);
    YogClass_define_descr_get_executor(env, cFunction, YogFunction_exec_get_descr);
    YogClass_define_descr_get_caller(env, cFunction, YogFunction_call_get_descr);
    YogClass_define_caller(env, cFunction, YogFunction_call);
    YogClass_define_executor(env, cFunction, YogFunction_exec);
#define DEFINE_METHOD(name, f)  do { \
    YogClass_define_method(env, cFunction, pkg, (name), (f)); \
} while (0)
    DEFINE_METHOD("descr_get", descr_get);
#undef DEFINE_METHOD
    vm->cFunction = cFunction;

    cInstanceMethod = YogClass_new(env, "InstanceMethod", vm->cObject);
    YogClass_define_allocator(env, cInstanceMethod, create_instance_method);
    YogClass_define_caller(env, cInstanceMethod, YogInstanceMethod_call);
    YogClass_define_executor(env, cInstanceMethod, YogInstanceMethod_exec);
    vm->cInstanceMethod = cInstanceMethod;

    cNativeInstanceMethod = YogClass_new(env, "NativeInstanceMethod", vm->cObject);
    YogClass_define_allocator(env, cNativeInstanceMethod, create_instance_method);
    YogClass_define_caller(env, cNativeInstanceMethod, YogNativeInstanceMethod_call);
    YogClass_define_executor(env, cNativeInstanceMethod, YogNativeInstanceMethod_exec);
    vm->cNativeInstanceMethod = cNativeInstanceMethod;

    RETURN_VOID(env);
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

    retval = call(env, self, argc, args, 0, NULL, YUNDEF, YUNDEF, YUNDEF);

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

    retval = call(env, self, argc, args, 0, NULL, YUNDEF, YUNDEF, block);

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
