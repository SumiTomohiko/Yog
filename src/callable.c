#include "yog/config.h"
#include <stdarg.h>
#include <string.h>
#include <strings.h>
#include "yog/arg.h"
#include "yog/array.h"
#include "yog/callable.h"
#include "yog/class.h"
#include "yog/code.h"
#include "yog/compat.h"
#include "yog/dict.h"
#include "yog/error.h"
#include "yog/eval.h"
#include "yog/frame.h"
#include "yog/handle.h"
#include "yog/misc.h"
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
    raise_TypeError(env, "*", "an Array", actual);
}

static void
raise_TypeError_for_varkwarg(YogEnv* env, YogVal actual)
{
    raise_TypeError(env, "**", "a Dict", actual);
}

#define STORE_LOCAL(env, frame, index, val) YogGC_UPDATE_PTR((env), HDL_AS(YogScriptFrame, (frame)), locals_etc[HDL_AS(YogScriptFrame, (frame))->stack_capacity + (index)], (val))

static void
assign_keyword_arg(YogEnv* env, YogHandle* self, uint_t args_offset, YogHandle* frame, YogHandle* kw, ID name, YogHandle* val)
{
    YogVal code = HDL_AS(YogFunction, self)->code;
    YogVal formal_args = PTR_AS(YogCode, code)->arg_info;
    uint_t argc = PTR_AS(YogArgInfo, formal_args)->argc;
    YogVal names = PTR_AS(YogArgInfo, formal_args)->argnames;
    uint_t i;
    for (i = 0; i < argc; i++) {
        if (PTR_AS(ID, names)[i] != name) {
            continue;
        }
        YogVal v = SCRIPT_FRAME_LOCALS(frame->val)[args_offset + i];
        if (!IS_UNDEF(v)) {
            YogError_raise_ArgumentError(env, "%I() got multiple values for keyword argument \"%I\"", HDL_AS(YogFunction, self)->name, name);
        }
        STORE_LOCAL(env, frame, args_offset + i, val->val);
        return;
    }
    if (!IS_PTR(kw->val)) {
        YogError_raise_ArgumentError(env, "an unexpected keyword argument \"%I\"", name);
    }
    YogDict_set(env, kw->val, ID2VAL(name), val->val);
}

static void
raise_wrong_num_args(YogEnv* env, YogVal self, uint_t posargc)
{
    YogVal code = PTR_AS(YogFunction, self)->code;
    YogVal formal_args = PTR_AS(YogCode, code)->arg_info;
    uint_t formal_posargc = PTR_AS(YogArgInfo, formal_args)->argc;
    YogError_raise_ArgumentError(env, "%I() requires %u positional argument(s) (%u given)", PTR_AS(YogFunction, self)->name, formal_posargc, posargc);
}

static void
check_arg_assigned(YogEnv* env, YogVal self, YogVal arg, uint_t posargc)
{
    if (!IS_UNDEF(arg)) {
        return;
    }
    raise_wrong_num_args(env, self, posargc);
}

static void
fill_args(YogEnv* env, YogHandle* self, uint_t args_offset, uint8_t posargc, YogHandle* posargs[], YogHandle* blockarg, uint8_t kwargc, YogHandle* kwargs[], YogHandle* vararg, YogHandle* varkwarg, YogHandle* frame)
{
    YOG_ASSERT(env, IS_PTR(self->val), "invalid self (0x%x)", self);
    YOG_ASSERT(env, BASIC_OBJ_TYPE(self->val) == TYPE_FUNCTION, "invalid type self (0x%x)", BASIC_OBJ_TYPE(self->val));
    YogVal code = HDL_AS(YogFunction, self)->code;
    YogVal a = PTR_AS(YogCode, code)->arg_info;
    if (!IS_PTR(a)) {
        return;
    }
    YogHandle* arg_info = YogHandle_REGISTER(env, a);

    YogHandle* kw = YogHandle_REGISTER(env, YUNDEF);
    uint_t arg_kwargc = HDL_AS(YogArgInfo, arg_info)->kwargc;
    if (0 < arg_kwargc) {
        uint_t index = HDL_AS(YogArgInfo, arg_info)->argc;
        uint_t varargc = HDL_AS(YogArgInfo, arg_info)->varargc;
        if (0 < varargc) {
            index++;
        }
        kw->val = YogDict_new(env);
        STORE_LOCAL(env, frame, args_offset + index, kw->val);
    }

    uint_t i;
    uint_t arg_argc = HDL_AS(YogArgInfo, arg_info)->argc;
    if (arg_argc < posargc) {
        uint_t argc = HDL_AS(YogArgInfo, arg_info)->argc;
        for (i = 0; i < argc; i++) {
            STORE_LOCAL(env, frame, args_offset + i, posargs[i]->val);
        }
        if (HDL_AS(YogArgInfo, arg_info)->varargc != 1) {
            raise_wrong_num_args(env, self->val, posargc);
        }
        YogHandle* array = YogHandle_REGISTER(env, YogArray_new(env));
        STORE_LOCAL(env, frame, args_offset + argc, array->val);
        for (i = argc; i < posargc; i++) {
            YogArray_push(env, array->val, posargs[i]->val);
        }
        if (IS_UNDEF(NULL2UNDEF(vararg))) {
            /* Do nothing */
        }
        else if (!IS_PTR(vararg->val) || (BASIC_OBJ_TYPE(vararg->val) != TYPE_ARRAY)) {
            raise_TypeError_for_vararg(env, vararg->val);
        }
        else {
            YogArray_extend(env, array->val, vararg->val);
        }
    }
    else {
        for (i = 0; i < posargc; i++) {
            STORE_LOCAL(env, frame, args_offset + i, posargs[i]->val);
        }
        if (IS_UNDEF(NULL2UNDEF(vararg))) {
            /* Do nothing */
        }
        else if (!IS_PTR(vararg->val) || (BASIC_OBJ_TYPE(vararg->val) != TYPE_ARRAY)) {
            raise_TypeError_for_vararg(env, vararg->val);
        }
        else {
            for (i = posargc; i < arg_argc; i++) {
                YogVal val = YogArray_at(env, vararg->val, i - posargc);
                STORE_LOCAL(env, frame, args_offset + i, val);
            }
        }
        if (0 < HDL_AS(YogArgInfo, arg_info)->varargc) {
            YogHandle* va = YogHandle_REGISTER(env, YogArray_new(env));
            STORE_LOCAL(env, frame, args_offset + posargc, va->val);

            if (IS_UNDEF(NULL2UNDEF(vararg))) {
                /* Do nothing */
            }
            else if (!IS_PTR(vararg->val) || (BASIC_OBJ_TYPE(vararg->val) != TYPE_ARRAY)) {
                raise_TypeError_for_vararg(env, vararg->val);
            }
            else {
                uint_t size = YogArray_size(env, vararg->val);
                for (i = arg_argc - posargc; i < size; i++) {
                    YogVal val = YogArray_at(env, vararg->val, i);
                    YogArray_push(env, va->val, val);
                }
            }
        }
    }

    for (i = 0; i < kwargc; i++) {
        ID name = VAL2ID(kwargs[2 * i]->val);
        YogHandle* val = kwargs[2 * i + 1];
        assign_keyword_arg(env, self, args_offset, frame, kw, name, val);
    }

    if (IS_UNDEF(NULL2UNDEF(varkwarg))) {
        /* Do nothing */
    }
    else if (!IS_PTR(varkwarg->val) || (BASIC_OBJ_TYPE(varkwarg->val) != TYPE_DICT)) {
        raise_TypeError_for_varkwarg(env, varkwarg->val);
    }
    else {
        YogHandle* iter = YogHandle_REGISTER(env, YogDict_get_iterator(env, varkwarg->val));
        while (YogDictIterator_next(env, iter->val)) {
            YogVal key = YogDictIterator_current_key(env, iter->val);
            if (!IS_SYMBOL(key)) {
                YogError_raise_TypeError(env, "keywords must be symbols, not %C", key);
            }
            YogHandle* val = YogHandle_REGISTER(env, YogDictIterator_current_value(env, iter->val));
            assign_keyword_arg(env, self, args_offset, frame, kw, VAL2ID(key), val);
        }
    }

    if (!IS_UNDEF(NULL2UNDEF(blockarg))) {
        if (HDL_AS(YogArgInfo, arg_info)->blockargc != 1) {
            YogError_raise_ArgumentError(env, "can't accept a block argument");
        }
        uint_t index = HDL_AS(YogArgInfo, arg_info)->argc;
        if (0 < HDL_AS(YogArgInfo, arg_info)->varargc) {
            index++;
        }
        if (0 < HDL_AS(YogArgInfo, arg_info)->kwargc) {
            index++;
        }
        STORE_LOCAL(env, frame, args_offset + index, blockarg->val);
    }

    uint_t required_argc = HDL_AS(YogArgInfo, arg_info)->required_argc;
    for (i = 0; i < required_argc; i++) {
        YogVal val = SCRIPT_FRAME_LOCALS(HDL2VAL(frame))[args_offset + i];
        check_arg_assigned(env, self->val, val, posargc);
    }
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
fill_outer_frames(YogEnv* env, YogVal frame, YogVal outer_frame, uint_t depth)
{
    if (depth == 0) {
        return;
    }
    YOG_ASSERT(env, depth <= PTR_AS(YogScriptFrame, frame)->outer_frames_num, "Shallow frame depth (%u, %u)", depth, PTR_AS(YogScriptFrame, frame)->outer_frames_num);

    uint_t offset = PTR_AS(YogScriptFrame, frame)->stack_capacity + PTR_AS(YogScriptFrame, frame)->locals_num;
    YogGC_UPDATE_PTR(env, PTR_AS(YogScriptFrame, frame), locals_etc[offset], outer_frame);
    uint_t i;
    for (i = 0; i < depth - 1; i++) {
        YogGC_UPDATE_PTR(env, PTR_AS(YogScriptFrame, frame), locals_etc[offset + 1 + i], SCRIPT_FRAME_OUTER_FRAMES(outer_frame)[i]);
    }
}

static void
YogFunction_exec_for_instance(YogEnv* env, YogHandle* callee, YogHandle* self, uint8_t posargc, YogHandle* posargs[], uint8_t kwargc, YogHandle* kwargs[], YogHandle* vararg, YogHandle* varkwarg, YogHandle* blockarg)
{
    YogVal code = HDL_AS(YogFunction, callee)->code;
    uint_t depth = PTR_AS(YogCode, code)->outer_size;
    uint_t locals_num = PTR_AS(YogCode, code)->local_vars_count;
    YogHandle* frame = YogHandle_REGISTER(env, YogFrame_get_script_frame(env, code, locals_num));
    uint_t args_offset;
    if (HDL_AS(YogFunction, callee)->needs_self) {
        uint_t pos = HDL_AS(YogScriptFrame, frame)->stack_capacity;
        YogGC_UPDATE_PTR(env, HDL_AS(YogScriptFrame, frame), locals_etc[pos], NULL2UNDEF(self));
        args_offset = 1;
    }
    else {
        args_offset = 0;
    }
    fill_args(env, callee, args_offset, posargc, posargs, blockarg, kwargc, kwargs, vararg, varkwarg, frame);

    YogGC_UPDATE_PTR(env, HDL_AS(YogScriptFrame, frame), globals, HDL_AS(YogFunction, callee)->globals);
    YogVal outer_frame = HDL_AS(YogFunction, callee)->outer_frame;
    fill_outer_frames(env, HDL2VAL(frame), outer_frame, depth);

    YogVal frame_to_long_return = HDL_AS(YogFunction, callee)->frame_to_long_return;
    YogGC_UPDATE_PTR(env, HDL_AS(YogScriptFrame, frame), frame_to_long_return, IS_PTR(frame_to_long_return) ? frame_to_long_return : env->frame);
    YogGC_UPDATE_PTR(env, HDL_AS(YogScriptFrame, frame), frame_to_long_break, HDL_AS(YogFunction, callee)->frame_to_long_break);

    if (!IS_UNDEF(NULL2UNDEF(self))) {
        YogGC_UPDATE_PTR(env, HDL_AS(YogScriptFrame, frame), klass, YogVal_get_class(env, self->val));
    }
    HDL_AS(YogScriptFrame, frame)->name = HDL_AS(YogFunction, callee)->name;

    YogEval_push_frame(env, HDL2VAL(frame));
}

static void
YogFunction_exec(YogEnv* env, YogHandle* callee, uint8_t posargc, YogHandle* posargs[], uint8_t kwargc, YogHandle* kwargs[], YogHandle* vararg, YogHandle* varkwarg, YogHandle* blockarg)
{
    YogFunction_exec_for_instance(env, callee, NULL, posargc, posargs, kwargc, kwargs, vararg, varkwarg, blockarg);
}

static YogVal
YogFunction_call_for_instance(YogEnv* env, YogHandle* callee, YogHandle* self, uint8_t posargc, YogHandle* posargs[], uint8_t kwargc, YogHandle* kwargs[], YogHandle* vararg, YogHandle* varkwarg, YogHandle* blockarg)
{
    YogEval_push_finish_frame(env);
    YogFunction_exec_for_instance(env, callee, self, posargc, posargs, kwargc, kwargs, vararg, varkwarg, blockarg);
    return YogEval_mainloop(env);
}

static YogVal
YogFunction_call(YogEnv* env, YogHandle* callee, uint8_t posargc, YogHandle* posargs[], uint8_t kwargc, YogHandle* kwargs[], YogHandle* vararg, YogHandle* varkwarg, YogHandle* blockarg)
{
    return YogFunction_call_for_instance(env, callee, NULL, posargc, posargs, kwargc, kwargs, vararg, varkwarg, blockarg);
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

static YogHandle*
create_keyword_argument(YogEnv* env, uint8_t kwargc, YogHandle* kwargs[], YogHandle* varkwarg)
{
    if ((kwargc == 0) && IS_UNDEF(NULL2UNDEF(varkwarg))) {
        return NULL;
    }

    YogHandle* kw = YogHandle_REGISTER(env, YogDict_new(env));
    uint_t i;
    for (i = 0; i < kwargc; i += 2) {
        YogDict_set(env, kw->val, kwargs[i]->val, kwargs[i + 1]->val);
    }
    if (IS_UNDEF(NULL2UNDEF(varkwarg))) {
    }
    else if (!IS_PTR(varkwarg->val) || (BASIC_OBJ_TYPE(varkwarg->val) != TYPE_DICT)) {
        raise_TypeError_for_varkwarg(env, varkwarg->val);
    }
    else {
        YogDict_add(env, kw->val, varkwarg->val);
    }

    return kw;
}

static YogHandle*
create_positional_argument(YogEnv* env, uint8_t posargc, YogHandle* posargs[], YogHandle* vararg)
{
    YogHandle* args = YogHandle_REGISTER(env, YogArray_new(env));
    uint_t i;
    for (i = 0; i < posargc; i++) {
        YogArray_push(env, HDL2VAL(args), HDL2VAL(posargs[i]));
    }

    if (IS_UNDEF(NULL2UNDEF(vararg))) {
    }
    else if (!IS_PTR(HDL2VAL(vararg)) || (BASIC_OBJ_TYPE(HDL2VAL(vararg)) != TYPE_ARRAY)) {
        raise_TypeError_for_vararg(env, HDL2VAL(vararg));
    }
    else {
        YogArray_add(env, HDL2VAL(args), HDL2VAL(vararg));
    }

    return args;
}

static YogHandle*
YogNativeFunction2_format_method(YogEnv* env, YogHandle* self)
{
    YogVal class_name = HDL_AS(YogNativeFunction2, self)->class_name;
    YogHandle* h_class_name = YogHandle_REGISTER(env, class_name);
    YogVal func_name = HDL_AS(YogNativeFunction2, self)->func_name;
    YogHandle* h_func_name = YogHandle_REGISTER(env, func_name);
    return YogMisc_format_method(env, h_class_name, h_func_name);
}

static void
set_posarg(YogEnv* env, YogHandle* self, uint_t args_num, YogHandle* args[], uint_t index, YogHandle* vararg, YogHandle* val, uint_t posargc)
{
    if (index < args_num) {
        args[index] = val;
        return;
    }
    if (vararg != NULL) {
        YogArray_push(env, HDL2VAL(vararg), HDL2VAL(val));
        return;
    }

    YogHandle* name = YogNativeFunction2_format_method(env, self);
    YogError_raise_ArgumentError(env, "%S() required %u positional argument(s) (%u given)", HDL2VAL(name), args_num, posargc);
    /* NOTREACHED */
}

static int_t
find_special_arg(YogEnv* env, YogHandle* self, ID name)
{
    uint_t args_num = HDL_AS(YogNativeFunction2, self)->args_num;
    YogNativeArg* args = HDL_AS(YogNativeFunction2, self)->args;
    uint_t i;
    for (i = 0; i < args_num; i++) {
        if (args[i].name != name) {
            continue;
        }
        return i;
    }
    return -1;
}

static int_t
find_varkwarg(YogEnv* env, YogHandle* self)
{
    return find_special_arg(env, self, env->vm->id_star);
}

static int_t
find_vararg(YogEnv* env, YogHandle* self)
{
    return find_special_arg(env, self, env->vm->id_star2);
}

static BOOL
find_keyword(YogEnv* env, YogHandle* self, uint_t argc, ID name, uint_t* pindex)
{
    YogNativeArg* args = HDL_AS(YogNativeFunction2, self)->args;
    uint_t i;
    for (i = 0; i < argc; i++) {
        if (args[i].name != name) {
            continue;
        }
        *pindex = i;
        return TRUE;
    }
    return FALSE;
}

static void
check_keyword_assigned(YogEnv* env, YogHandle* self, YogHandle* val, ID name)
{
    if (val == NULL) {
        return;
    }
    YogHandle* meth = YogNativeFunction2_format_method(env, self);
    YogError_raise_ArgumentError(env, "%S() got multiple values for keyword argument \"%I\"", HDL2VAL(meth), name);
}

static void
set_keyword_arg(YogEnv* env, YogHandle* self, uint_t argc, YogHandle* args[], YogHandle* varkwarg, ID name, YogHandle* val)
{
    uint_t index;
    if (find_keyword(env, self, argc, name, &index)) {
        check_keyword_assigned(env, self, args[index], name);
        args[index] = val;
        return;
    }
    if (varkwarg != NULL) {
        YogDict_set(env, HDL2VAL(varkwarg), ID2VAL(name), HDL2VAL(val));
        return;
    }
    YogHandle* f = YogNativeFunction2_format_method(env, self);
    const char* fmt = "%S() got an unexpected keyword argument \"%I\"";
    YogError_raise_ArgumentError(env, fmt, HDL2VAL(f), name);
}

static void
set_vararg_each(YogEnv* env, YogHandle* self, uint_t argc, YogHandle* args[], uint_t index, YogHandle* vararg, YogVal val, uint_t given_argc)
{
    if (index < argc) {
        args[index] = YogHandle_REGISTER(env, val);
        return;
    }
    if (vararg != NULL) {
        YogArray_push(env, HDL2VAL(vararg), val);
        return;
    }

    YogHandle* name = YogNativeFunction2_format_method(env, self);
    YogError_raise_ArgumentError(env, "%S() takes exactly %u argument(s) (%u given)", HDL2VAL(name), argc, given_argc);
    /* NOTREACHED */
}

static void
set_vararg(YogEnv* env, YogHandle* self, uint_t argc, YogHandle* args[], YogHandle* vararg, YogHandle* arg, uint_t posargc)
{
    uint_t size = YogArray_size(env, HDL2VAL(arg));
    uint_t given_argc = size + posargc;
    uint_t i;
    for (i = 0; i < size; i++) {
        YogVal val = YogArray_at(env, HDL2VAL(arg), i);
        uint_t index = posargc + i;
        set_vararg_each(env, self, argc, args, index, vararg, val, given_argc);
    }
}

static void
set_varkwarg(YogEnv* env, YogHandle* self, uint_t argc, YogHandle* args[], YogHandle* varkwarg, YogHandle* arg)
{
    YogVal iter = YogDict_get_iterator(env, HDL2VAL(arg));
    YogHandle* h_iter = YogHandle_REGISTER(env, iter);
    while (YogDictIterator_next(env, HDL2VAL(h_iter))) {
        YogVal name = YogDictIterator_current_key(env, HDL2VAL(h_iter));
        YogVal val = YogDictIterator_current_value(env, HDL2VAL(h_iter));
        YogHandle* h_val = YogHandle_REGISTER(env, val);
        set_keyword_arg(env, self, argc, args, varkwarg, VAL2ID(name), h_val);
    }
}

static void
set_blockarg(YogEnv* env, YogHandle* self, YogHandle* args[], YogHandle* blockarg)
{
    uint_t args_num = HDL_AS(YogNativeFunction2, self)->args_num;
    YogNativeArg* formal_args = HDL_AS(YogNativeFunction2, self)->args;
    if (formal_args[args_num - 1].name != env->vm->id_amp) {
        YogHandle* name = YogNativeFunction2_format_method(env, self);
        YogError_raise_ArgumentError(env, "%S got an unexpected block argument", HDL2VAL(name));
        /* NOTREACHED */
    }
    args[args_num - 1] = blockarg;
}

static YogVal
YogNativeFunction2_call_for_instance(YogEnv* env, YogHandle* self, YogHandle* recv, uint8_t posargc, YogHandle* posargs[], uint8_t kwargc, YogHandle* kwargs[], YogHandle* vararg, YogHandle* varkwarg, YogHandle* blockarg, YogVal* pmulti_val)
{
    YogVal frame = YogFrame_get_c_frame(env);
    YogGC_UPDATE_PTR(env, PTR_AS(YogCFrame, frame), f, HDL2VAL(self));
    YogEval_push_frame(env, frame);

    uint_t args_num = HDL_AS(YogNativeFunction2, self)->args_num;
    YogHandle* args[args_num];
    bzero(args, sizeof(args));

    uint_t formal_posargc = HDL_AS(YogNativeFunction2, self)->posargs_num;
    YogHandle* formal_vararg;
    if (HDL_AS(YogNativeFunction2, self)->vararg_pos != -1) {
        formal_vararg = YogHandle_REGISTER(env, YogArray_new(env));
        args[HDL_AS(YogNativeFunction2, self)->vararg_pos] = formal_vararg;
    }
    else {
        formal_vararg = NULL;
    }
    uint_t i;
    for (i = 0; i < posargc; i++) {
        set_posarg(env, self, formal_posargc, args, i, formal_vararg, posargs[i], posargc);
    }

    if (vararg != NULL) {
        set_vararg(env, self, formal_posargc, args, formal_vararg, vararg, posargc);
    }

    YogHandle* formal_varkwarg;
    if (HDL_AS(YogNativeFunction2, self)->varkwarg_pos != -1) {
        formal_varkwarg = YogHandle_REGISTER(env, YogDict_new(env));
        args[HDL_AS(YogNativeFunction2, self)->varkwarg_pos] = formal_varkwarg;
    }
    else {
        formal_varkwarg = NULL;
    }
    for (i = 0; i < kwargc; i += 2) {
        ID name = VAL2ID(HDL2VAL(kwargs[i]));
        YogHandle* val = kwargs[i + 1];
        set_keyword_arg(env, self, formal_posargc, args, formal_varkwarg, name, val);
    }

    if (varkwarg != NULL) {
        set_varkwarg(env, self, formal_posargc, args, formal_varkwarg, varkwarg);
    }

    if (blockarg != NULL) {
        set_blockarg(env, self, args, blockarg);
    }

    void* f = HDL_AS(YogNativeFunction2, self)->f;
    YogVal pkg = HDL_AS(YogNativeFunction2, self)->pkg;
    YogHandle* h_pkg = YogHandle_REGISTER(env, pkg);
    /* gcc cannot detect that retval is always initialized */
    YogVal retval = YUNDEF;
    switch (args_num) {
    case 0:
        retval = ((YogVal (*)(YogEnv*, YogHandle*, YogHandle*))f)(env, recv, h_pkg);
        break;
    case 1:
        retval = ((YogVal (*)(YogEnv*, YogHandle*, YogHandle*, YogHandle*))f)(env, recv, h_pkg, args[0]);
        break;
    case 2:
        retval = ((YogVal (*)(YogEnv*, YogHandle*, YogHandle*, YogHandle*, YogHandle*))f)(env, recv, h_pkg, args[0], args[1]);
        break;
    case 3:
        retval = ((YogVal (*)(YogEnv*, YogHandle*, YogHandle*, YogHandle*, YogHandle*, YogHandle*))f)(env, recv, h_pkg, args[0], args[1], args[2]);
        break;
    case 4:
        retval = ((YogVal (*)(YogEnv*, YogHandle*, YogHandle*, YogHandle*, YogHandle*, YogHandle*, YogHandle*))f)(env, recv, h_pkg, args[0], args[1], args[2], args[3]);
        break;
    case 5:
        retval = ((YogVal (*)(YogEnv*, YogHandle*, YogHandle*, YogHandle*, YogHandle*, YogHandle*, YogHandle*, YogHandle*))f)(env, recv, h_pkg, args[0], args[1], args[2], args[3], args[4]);
        break;
    case 6:
        retval = ((YogVal (*)(YogEnv*, YogHandle*, YogHandle*, YogHandle*, YogHandle*, YogHandle*, YogHandle*, YogHandle*, YogHandle*))f)(env, recv, h_pkg, args[0], args[1], args[2], args[3], args[4], args[5]);
        break;
    case 7:
        retval = ((YogVal (*)(YogEnv*, YogHandle*, YogHandle*, YogHandle*, YogHandle*, YogHandle*, YogHandle*, YogHandle*, YogHandle*, YogHandle*))f)(env, recv, h_pkg, args[0], args[1], args[2], args[3], args[4], args[5], args[6]);
        break;
    case 8:
        retval = ((YogVal (*)(YogEnv*, YogHandle*, YogHandle*, YogHandle*, YogHandle*, YogHandle*, YogHandle*, YogHandle*, YogHandle*, YogHandle*, YogHandle*))f)(env, recv, h_pkg, args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7]);
        break;
    default:
        YOG_BUG(env, "Can't handle more than 8 arguments");
        /* NOTREACHED */
        break;
    }

    if (IS_UNDEF(retval) && (pmulti_val != NULL)) {
        *pmulti_val = PTR_AS(YogCFrame, env->frame)->multi_val;
    }

    YogThread_put_c_frame(env, env->thread, env->frame);
    YogEval_pop_frame(env);
    return retval;
}

static YogVal
YogNativeFunction_call_for_instance(YogEnv* env, YogHandle* callee, YogHandle* self, uint8_t posargc, YogHandle* posargs[], uint8_t kwargc, YogHandle* kwargs[], YogHandle* vararg, YogHandle* varkwarg, YogHandle* blockarg, YogVal* multi_val)
{
    YogHandle* args = create_positional_argument(env, posargc, posargs, vararg);
    YogHandle* kw = create_keyword_argument(env, kwargc, kwargs, varkwarg);

    YogHandle* frame = YogHandle_REGISTER(env, YogFrame_get_c_frame(env));
    YogGC_UPDATE_PTR(env, HDL_AS(YogCFrame, frame), f, HDL2VAL(callee));
    YogEval_push_frame(env, HDL2VAL(frame));

    YogNativeFunction* obj = HDL_AS(YogNativeFunction, callee);
    Body f = (Body)obj->f;
    YogVal retval = (*f)(env, NULL2UNDEF(self), obj->pkg, HDL2VAL(args), NULL2UNDEF(kw), NULL2UNDEF(blockarg));
    if (IS_UNDEF(retval) && (multi_val != NULL)) {
        *multi_val = HDL_AS(YogCFrame, frame)->multi_val;
    }

    YogThread_put_c_frame(env, env->thread, env->frame);
    YogEval_pop_frame(env);
    return retval;
}

static void
error_for_multi_value(YogEnv* env, YogVal retval)
{
    if (!IS_UNDEF(retval)) {
        return;
    }
    YogError_raise_ValueError(env, "multiple value are not allowed");
}

static void
YogNativeFunction2_exec_for_instance(YogEnv* env, YogHandle* self, YogHandle* recv, uint8_t posargc, YogHandle* posargs[], uint8_t kwargc, YogHandle* kwargs[], YogHandle* vararg, YogHandle* varkwarg, YogHandle* blockarg)
{
    YogVal multi_val;
    YogVal retval = YogNativeFunction2_call_for_instance(env, self, recv, posargc, posargs, kwargc, kwargs, vararg, varkwarg, blockarg, &multi_val);
    if (IS_UNDEF(retval)) {
        YogEval_push_returned_multi_value(env, multi_val);
        return;
    }
    YogEval_push_returned_value(env, env->frame, retval);
}

static void
YogNativeFunction2_exec(YogEnv* env, YogHandle* self, uint8_t posargc, YogHandle* posargs[], uint8_t kwargc, YogHandle* kwargs[], YogHandle* vararg, YogHandle* varkwarg, YogHandle* blockarg)
{
    YogNativeFunction2_exec_for_instance(env, self, NULL, posargc, posargs, kwargc, kwargs, vararg, varkwarg, blockarg);
}

static YogVal
YogNativeFunction2_call(YogEnv* env, YogHandle* callee, uint8_t posargc, YogHandle* posargs[], uint8_t kwargc, YogHandle* kwargs[], YogHandle* vararg, YogHandle* varkwarg, YogHandle* blockarg)
{
    YogVal retval = YogNativeFunction2_call_for_instance(env, callee, NULL, posargc, posargs, kwargc, kwargs, vararg, varkwarg, blockarg, NULL);
    error_for_multi_value(env, retval);
    return retval;
}

static YogVal
YogNativeFunction_call(YogEnv* env, YogHandle* callee, uint8_t posargc, YogHandle* posargs[], uint8_t kwargc, YogHandle* kwargs[], YogHandle* vararg, YogHandle* varkwarg, YogHandle* blockarg)
{
    YogVal retval = YogNativeFunction_call_for_instance(env, callee, NULL, posargc, posargs, kwargc, kwargs, vararg, varkwarg, blockarg, NULL);
    error_for_multi_value(env, retval);
    return retval;
}

static void
YogNativeFunction_exec_for_instance(YogEnv* env, YogHandle* callee, YogHandle* self, uint8_t posargc, YogHandle* posargs[], uint8_t kwargc, YogHandle* kwargs[], YogHandle* vararg, YogHandle* varkwarg, YogHandle* blockarg)
{
    YogVal multi_val;
    YogVal retval = YogNativeFunction_call_for_instance(env, callee, self, posargc, posargs, kwargc, kwargs, vararg, varkwarg, blockarg, &multi_val);
    if (IS_UNDEF(retval)) {
        YogEval_push_returned_multi_value(env, multi_val);
        return;
    }
    YogEval_push_returned_value(env, env->frame, retval);
}

static void
YogNativeFunction_exec(YogEnv* env, YogHandle* callee, uint8_t posargc, YogHandle* posargs[], uint8_t kwargc, YogHandle* kwargs[], YogHandle* vararg, YogHandle* varkwarg, YogHandle* blockarg)
{
    YogNativeFunction_exec_for_instance(env, callee, NULL, posargc, posargs, kwargc, kwargs, vararg, varkwarg, blockarg);
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
YogInstanceMethod_exec(YogEnv* env, YogHandle* callee, uint8_t posargc, YogHandle* posargs[], uint8_t kwargc, YogHandle* kwargs[], YogHandle* vararg, YogHandle* varkwarg, YogHandle* blockarg)
{
    YogInstanceMethod* obj = HDL_AS(YogInstanceMethod, callee);
    YogHandle* self = YogHandle_REGISTER(env, obj->self);
    YogHandle* f = YogHandle_REGISTER(env, obj->f);
    YogFunction_exec_for_instance(env, f, self, posargc, posargs, kwargc, kwargs, vararg, varkwarg, blockarg);
}

static YogVal
YogInstanceMethod_call(YogEnv* env, YogHandle* callee, uint8_t posargc, YogHandle* posargs[], uint8_t kwargc, YogHandle* kwargs[], YogHandle* vararg, YogHandle* varkwarg, YogHandle* blockarg)
{
    YogInstanceMethod* obj = HDL_AS(YogInstanceMethod, callee);
    YogHandle* self = YogHandle_REGISTER(env, obj->self);
    YogHandle* f = YogHandle_REGISTER(env, obj->f);
    return YogFunction_call_for_instance(env, f, self, posargc, posargs, kwargc, kwargs, vararg, varkwarg, blockarg);
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
YogInstanceMethod_new(YogEnv* env)
{
    return create_instance_method(env, env->vm->cInstanceMethod);
}

static void
YogNativeInstanceMethod2_exec(YogEnv* env, YogHandle* self, uint8_t posargc, YogHandle* posargs[], uint8_t kwargc, YogHandle* kwargs[], YogHandle* vararg, YogHandle* varkwarg, YogHandle* blockarg)
{
    YogInstanceMethod* obj = HDL_AS(YogInstanceMethod, self);
    YogHandle* recv = YogHandle_REGISTER(env, obj->self);
    YogHandle* f = YogHandle_REGISTER(env, obj->f);
    YogNativeFunction2_exec_for_instance(env, f, recv, posargc, posargs, kwargc, kwargs, vararg, varkwarg, blockarg);
}

static void
YogNativeInstanceMethod_exec(YogEnv* env, YogHandle* callee, uint8_t posargc, YogHandle* posargs[], uint8_t kwargc, YogHandle* kwargs[], YogHandle* vararg, YogHandle* varkwarg, YogHandle* blockarg)
{
    YogInstanceMethod* obj = HDL_AS(YogInstanceMethod, callee);
    YogHandle* self = YogHandle_REGISTER(env, obj->self);
    YogHandle* f = YogHandle_REGISTER(env, obj->f);
    YogNativeFunction_exec_for_instance(env, f, self, posargc, posargs, kwargc, kwargs, vararg, varkwarg, blockarg);
}

static YogVal
YogNativeInstanceMethod2_call(YogEnv* env, YogHandle* self, uint8_t posargc, YogHandle* posargs[], uint8_t kwargc, YogHandle* kwargs[], YogHandle* vararg, YogHandle* varkwarg, YogHandle* blockarg)
{
    YogInstanceMethod* obj = HDL_AS(YogInstanceMethod, self);
    YogHandle* recv = YogHandle_REGISTER(env, obj->self);
    YogHandle* f = YogHandle_REGISTER(env, obj->f);
    YogVal retval = YogNativeFunction2_call_for_instance(env, f, recv, posargc, posargs, kwargc, kwargs, vararg, varkwarg, blockarg, NULL);
    error_for_multi_value(env, retval);
    return retval;
}

static YogVal
YogNativeInstanceMethod_call(YogEnv* env, YogHandle* callee, uint8_t posargc, YogHandle* posargs[], uint8_t kwargc, YogHandle* kwargs[], YogHandle* vararg, YogHandle* varkwarg, YogHandle* blockarg)
{
    YogInstanceMethod* obj = HDL_AS(YogInstanceMethod, callee);
    YogHandle* self = YogHandle_REGISTER(env, obj->self);
    YogHandle* f = YogHandle_REGISTER(env, obj->f);
    YogVal retval = YogNativeFunction_call_for_instance(env, f, self, posargc, posargs, kwargc, kwargs, vararg, varkwarg, blockarg, NULL);
    error_for_multi_value(env, retval);
    return retval;
}

static YogVal
YogNativeInstanceMethod_new(YogEnv* env)
{
    return create_instance_method(env, env->vm->cNativeInstanceMethod);
}

static YogVal
YogNativeInstanceMethod2_new(YogEnv* env)
{
    return create_instance_method(env, env->vm->cNativeInstanceMethod2);
}

static YogVal
YogNativeFunction2_call_get_descr(YogEnv* env, YogVal self, YogVal obj, YogVal klass)
{
    YogHandle* h_self = YogHandle_REGISTER(env, self);
    YogHandle* h_obj = YogHandle_REGISTER(env, obj);
    YogVal meth = YogNativeInstanceMethod2_new(env);
    YogGC_UPDATE_PTR(env, PTR_AS(YogInstanceMethod, meth), self, HDL2VAL(h_obj));
    YogGC_UPDATE_PTR(env, PTR_AS(YogInstanceMethod, meth), f, HDL2VAL(h_self));
    return meth;
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
YogNativeFunction2_exec_get_descr(YogEnv* env, YogVal self, YogVal obj, YogVal klass)
{
    YogVal retval = YogNativeFunction2_call_get_descr(env, self, obj, klass);
    YogScriptFrame_push_stack(env, env->frame, retval);
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

static uint_t
count_args(YogEnv* env, va_list ap)
{
    uint_t n = 0;
    va_list aq;
    va_copy(aq, ap);
    const char* name;
    while ((name = va_arg(aq, const char*)) != NULL) {
        n += strcmp(name, "|") == 0 ? 0 : 1;
    }
    return n;
}

static void
YogNativeFunction2_keep_children(YogEnv* env, void* ptr, ObjectKeeper keeper, void* heap)
{
    YogBasicObj_keep_children(env, ptr, keeper, heap);

    YogNativeFunction2* f = (YogNativeFunction2*)ptr;
#define KEEP(name) YogGC_KEEP(env, f, name, keeper, heap)
    KEEP(pkg);
    KEEP(class_name);
    KEEP(func_name);
#undef KEEP
}

static uint_t
init_arg(YogEnv* env, YogHandle* f, uint_t index, const char* name, BOOL* optional)
{
    if (strcmp(name, "|") == 0) {
        *optional = TRUE;
        return 0;
    }
    ID id = YogVM_intern(env, env->vm, name);
    YogNativeArg* arg = &HDL_AS(YogNativeFunction2, f)->args[index];
    arg->name = id;
    arg->optional = *optional;
    return 1;
}

YogHandle*
YogNativeFunction2_new(YogEnv* env, YogHandle* pkg, YogHandle* class_name, YogHandle* func_name, void* f, va_list ap)
{
    uint_t args_num = count_args(env, ap);
    YogVal self = ALLOC_OBJ_ITEM(env, YogNativeFunction2_keep_children, NULL, YogNativeFunction2, args_num, YogNativeArg);
    YogHandle* h_self = YogHandle_REGISTER(env, self);
    YogVal klass = env->vm->cNativeFunction2;
    YogBasicObj_init(env, self, TYPE_NATIVE_FUNCTION2, 0, klass);
    YogGC_UPDATE_PTR(env, HDL_AS(YogNativeFunction2, h_self), pkg, HDL2VAL(pkg));
    YogGC_UPDATE_PTR(env, HDL_AS(YogNativeFunction2, h_self), class_name, HDL2VAL(class_name));
    YogGC_UPDATE_PTR(env, HDL_AS(YogNativeFunction2, h_self), func_name, HDL2VAL(func_name));
    HDL_AS(YogNativeFunction2, h_self)->f = f;
    HDL_AS(YogNativeFunction2, h_self)->args_num = args_num;
    va_list aq;
    va_copy(aq, ap);
    uint_t n = 0;
    BOOL optional = FALSE;
    const char* name;
    while ((name = va_arg(ap, const char*)) != NULL) {
        n += init_arg(env, h_self, n, name, &optional);
    }
    uint_t posargs_num = 0;
    while ((name = va_arg(aq, const char*)) != NULL) {
        char c = name[0];
        posargs_num += (c == '|') || (c == '*') || (c == '&') ? 0 : 1;
    }
    HDL_AS(YogNativeFunction2, h_self)->posargs_num = posargs_num;
    HDL_AS(YogNativeFunction2, h_self)->vararg_pos = find_vararg(env, h_self);
    HDL_AS(YogNativeFunction2, h_self)->varkwarg_pos = find_varkwarg(env, h_self);

    return h_self;
}

void
YogFunction_define_classes(YogEnv* env, YogVal pkg)
{
    SAVE_ARG(env, pkg);
    YogVal cNativeFunction = YUNDEF;
    YogVal cNativeFunction2 = YUNDEF;
    YogVal cFunction = YUNDEF;
    YogVal cInstanceMethod = YUNDEF;
    YogVal cNativeInstanceMethod = YUNDEF;
    YogVal cNativeInstanceMethod2 = YUNDEF;
    PUSH_LOCALS6(env, cNativeFunction, cNativeFunction2, cFunction, cInstanceMethod, cNativeInstanceMethod, cNativeInstanceMethod2);
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

    cNativeFunction2 = YogClass_new(env, "NativeFunction2", vm->cObject);
    YogClass_define_descr_get_executor(env, cNativeFunction2, YogNativeFunction2_exec_get_descr);
    YogClass_define_descr_get_caller(env, cNativeFunction2, YogNativeFunction2_call_get_descr);
    YogClass_define_caller(env, cNativeFunction2, YogNativeFunction2_call);
    YogClass_define_executor(env, cNativeFunction2, YogNativeFunction2_exec);
#define DEFINE_METHOD(name, f)  do { \
    YogClass_define_method(env, cNativeFunction2, pkg, (name), (f)); \
} while (0)
    DEFINE_METHOD("descr_get", descr_get);
#undef DEFINE_METHOD
    vm->cNativeFunction2 = cNativeFunction2;

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

    cNativeInstanceMethod2 = YogClass_new(env, "NativeInstanceMethod2", vm->cObject);
    YogClass_define_caller(env, cNativeInstanceMethod2, YogNativeInstanceMethod2_call);
    YogClass_define_executor(env, cNativeInstanceMethod2, YogNativeInstanceMethod2_exec);
    vm->cNativeInstanceMethod2 = cNativeInstanceMethod2;

    RETURN_VOID(env);
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
YogCallable_call_with_block(YogEnv* env, YogVal self, uint_t argc, YogVal* args, YogVal block)
{
    YogHandleScope scope;
    YogHandleScope_OPEN(env, &scope);

    YogVal klass = YogVal_get_class(env, self);
    Caller call = PTR_AS(YogClass, klass)->call;
    YOG_ASSERT(env, call != NULL, "uncallable");

    YogHandle* h_self = YogHandle_REGISTER(env, self);
    YogHandle* h_args[argc];
    uint_t i;
    for (i = 0; i < argc; i++) {
        h_args[i] = YogHandle_REGISTER(env, args[i]);
    }
    YogHandle* h_block = IS_UNDEF(block) ? NULL : YogHandle_REGISTER(env, block);
    YogVal val = call(env, h_self, argc, h_args, 0, NULL, NULL, NULL, h_block);

    YogHandleScope_close(env);

    return val;
}

YogVal
YogCallable_call(YogEnv* env, YogVal self, uint_t argc, YogVal* args)
{
    return YogCallable_call_with_block(env, self, argc, args, YUNDEF);
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
