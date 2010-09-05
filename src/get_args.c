#include <string.h>
#include "yog/array.h"
#include "yog/binary.h"
#include "yog/dict.h"
#include "yog/error.h"
#include "yog/get_args.h"
#include "yog/handle.h"
#include "yog/string.h"
#include "yog/vm.h"
#include "yog/yog.h"

#define FINISHED(param)     (((param)->name == NULL) || (strcmp((param)->name, "*") == 0) || (strcmp((param)->name, "**") == 0))

static BOOL
find_param(YogEnv* env, YogCArg* params, YogVal name)
{
    SAVE_ARG(env, name);

    YogCArg* param = params;
    while (!FINISHED(param)) {
        if (strcmp(param->name, "|") == 0) {
            param++;
            YOG_ASSERT(env, param->name != NULL, "invalid format");
        }
        YogHandle* h = YogHandle_REGISTER(env, name);
        YogVal bin = YogString_to_bin_in_default_encoding(env, h);
        if (strcmp(BINARY_CSTR(bin), param->name) == 0) {
            RETURN(env, TRUE);
        }
        param++;
    }

    RETURN(env, FALSE);
}

static void
copy_dict(YogEnv* env, YogVal dest, YogVal src, YogCArg* params)
{
    SAVE_ARGS2(env, dest, src);
    YogVal iter = YUNDEF;
    YogVal key = YUNDEF;
    YogVal val = YUNDEF;
    YogVal name = YUNDEF;
    PUSH_LOCALS4(env, iter, key, val, name);

    if (!IS_PTR(src)) {
        RETURN_VOID(env);
    }

    iter = YogDict_get_iterator(env, src);
    while (YogDictIterator_next(env, iter)) {
        key = YogDictIterator_current_key(env, iter);
        YOG_ASSERT(env, IS_SYMBOL(key), "key is not symbol (0x%08x)", key);
        name = YogVM_id2name(env, env->vm, key);
        if (!find_param(env, params, name)) {
            val = YogDictIterator_current_value(env, iter);
            YogDict_set(env, dest, key, val);
        }
    }

    RETURN_VOID(env);
}

static void
set_positional_arg(YogEnv* env, YogCArg* param, uint_t pos, YogVal args, YogVal kw)
{
    SAVE_ARGS2(env, args, kw);

    ID name = YogVM_intern(env, env->vm, param->name);
    if (IS_PTR(kw) && YogDict_include(env, kw, ID2VAL(name))) {
        YogError_raise_TypeError(env, "Argument given by name (\"%s\") and position (%u)", param->name, pos + 1);
    }
    *param->dest = YogArray_at(env, args, pos);

    RETURN_VOID(env);
}

static void
set_keyword_arg(YogEnv* env, YogCArg* param, BOOL optional, YogVal kw)
{
    SAVE_ARG(env, kw);
    YogVal val = YUNDEF;
    PUSH_LOCAL(env, val);

    ID name = YogVM_intern(env, env->vm, param->name);
    val = IS_PTR(kw) ? YogDict_get(env, kw, ID2VAL(name)) : YUNDEF;
    if (!IS_UNDEF(val)) {
        *param->dest = val;
    }
    else if (!optional) {
        YogError_raise_TypeError(env, "Required argument \"%s\" not found", param->name);
    }

    RETURN_VOID(env);
}

static void
copy_array(YogEnv* env, YogVal dest, YogVal src, uint_t init_pos)
{
    SAVE_ARGS2(env, dest, src);
    YogVal val = YUNDEF;
    PUSH_LOCAL(env, val);

    if (!IS_PTR(src)) {
        RETURN_VOID(env);
    }

    uint_t size = YogArray_size(env, src);
    uint_t pos = init_pos;
    while (pos < size) {
        val = YogArray_at(env, src, pos);
        YogArray_push(env, dest, val);
        pos++;
    }

    RETURN_VOID(env);
}

static YogVal
find_invalid_keyword(YogEnv* env, YogCArg* params, YogVal kw)
{
    SAVE_ARG(env, kw);
    YogVal iter = YUNDEF;
    YogVal key = YUNDEF;
    YogVal name = YUNDEF;
    PUSH_LOCALS3(env, iter, key, name);

    if (!IS_PTR(kw)) {
        RETURN(env, YUNDEF);
    }

    iter = YogDict_get_iterator(env, kw);
    while (YogDictIterator_next(env, iter)) {
        key = YogDictIterator_current_key(env, iter);
        YOG_ASSERT(env, IS_SYMBOL(key), "key is not symbol (0x%08x)", key);
        name = YogVM_id2name(env, env->vm, VAL2ID(key));
        if (!find_param(env, params, name)) {
            RETURN(env, key);
        }
    }

    RETURN(env, YUNDEF);
}

void
YogGetArgs_parse_args(YogEnv* env, const char* func_name, YogCArg* params, YogVal args, YogVal kw)
{
    SAVE_ARGS2(env, args, kw);
    YogVal dest = YUNDEF;
    YogVal invalid_key = YUNDEF;
    PUSH_LOCALS2(env, dest, invalid_key);

#define ACCEPT_OPT_MARK(param)  do { \
    if (strcmp((param)->name, "|") == 0) { \
        optional = TRUE; \
        (param)++; \
    } \
} while (0)
    BOOL optional = FALSE;
    YogCArg* param = params;
    uint_t args_index = 0;
    uint_t args_size = IS_PTR(args) ? YogArray_size(env, args) : 0;
    while (!FINISHED(param) && (args_index < args_size)) {
        ACCEPT_OPT_MARK(param);
        set_positional_arg(env, param, args_index, args, kw);
        args_index++;
        param++;
    }

    uint_t kw_num = 0;
    while (!FINISHED(param)) {
        ACCEPT_OPT_MARK(param);
        set_keyword_arg(env, param, optional, kw);
        kw_num++;
        param++;
    }
#undef ACCEPT_OPT_MARK

    if ((param->name != NULL) && (strcmp(param->name, "*") == 0)) {
        dest = YogArray_new(env);
        copy_array(env, dest, args, args_index);
        *(YogVal*)param->dest = dest;
        param++;
        if (param->name == NULL) {
            RETURN_VOID(env);
        }
    }
    else if (args_index < args_size) {
        YogError_raise_TypeError(env, "%s() takes at most %u argument(s) (%u given)", func_name, args_index, args_size);
    }

    if ((param->name != NULL) && (strcmp(param->name, "**") == 0)) {
        dest = YogDict_new(env);
        copy_dict(env, dest, kw, params);
        *(YogVal*)param->dest = dest;
        RETURN_VOID(env);
    }

    uint_t kw_size = IS_PTR(kw) ? YogDict_size(env, kw) : 0;
    if (args_index + kw_num < args_size + kw_size) {
        YogError_raise_TypeError(env, "%s() takes at most %u argument(s) (%u given)", func_name, args_index + kw_num, args_size + kw_size);
    }
    invalid_key = find_invalid_keyword(env, params, kw);
    if (IS_SYMBOL(invalid_key)) {
        YogError_raise_TypeError(env, "\"%I\" is an invalid keyword argument for this function", invalid_key);
    }

    RETURN_VOID(env);
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
