#include "yog/config.h"
#include <string.h>
#include "yog/array.h"
#include "yog/callable.h"
#include "yog/class.h"
#include "yog/error.h"
#include "yog/eval.h"
#include "yog/frame.h"
#include "yog/gc.h"
#include "yog/get_args.h"
#include "yog/handle.h"
#include "yog/misc.h"
#include "yog/object.h"
#include "yog/string.h"
#include "yog/vm.h"
#include "yog/yog.h"

#define CHECK_SELF_TYPE(env, self)  do { \
    if (IS_PTR(self) && (BASIC_OBJ_TYPE(self) != TYPE_ARRAY)) { \
        YogError_raise_TypeError((env), "self must be Array"); \
    } \
} while (0)
#define CHECK_SELF_TYPE2(env, self)  do { \
    YogVal a = HDL2VAL((self)); \
    if (IS_PTR(a) && (BASIC_OBJ_TYPE(a) != TYPE_ARRAY)) { \
        YogError_raise_TypeError((env), "self must be Array"); \
    } \
} while (0)

uint_t
YogValArray_size(YogEnv* env, YogVal array)
{
    return PTR_AS(YogValArray, array)->size;
}

YogVal
YogValArray_at(YogEnv* env, YogVal array, uint_t n)
{
    uint_t size = PTR_AS(YogValArray, array)->size;
    YOG_ASSERT(env, n < size, "index exceed array body size (%u, %u)", n, size);
    return PTR_AS(YogValArray, array)->items[n];
}

YogVal
YogArray_at(YogEnv* env, YogVal array, uint_t n)
{
    return YogValArray_at(env, PTR_AS(YogArray, array)->body, n);
}

uint_t
YogArray_size(YogEnv* env, YogVal array)
{
    return PTR_AS(YogArray, array)->size;
}

static void
YogValArray_keep_children(YogEnv* env, void* ptr, ObjectKeeper keeper, void* heap)
{
    YogValArray* array = PTR_AS(YogValArray, ptr);

    uint_t size = array->size;
    uint_t i = 0;
    for (i = 0; i < size; i++) {
        YogGC_KEEP(env, array, items[i], keeper, heap);
    }
}

YogVal
YogValArray_new(YogEnv* env, uint_t size)
{
    YogGC_check_multiply_overflow(env, size, sizeof(YogVal));
    YogVal array = ALLOC_OBJ_ITEM(env, YogValArray_keep_children, NULL, YogValArray, size, YogVal);
    PTR_AS(YogValArray, array)->size = size;
	uint_t i;
    for (i = 0; i < size; i++) {
        PTR_AS(YogValArray, array)->items[i] = YNIL;
    }

    return array;
}

static uint_t
multiple_size(uint_t min_size, uint_t cur_size, uint_t ratio)
{
    while (cur_size < min_size) {
        cur_size *= ratio;
    }
    return cur_size;
}

static uint_t
get_next_size(uint_t min_size, uint_t cur_size, uint_t ratio)
{
    if (cur_size == 0) {
        return 1;
    }

    return multiple_size(min_size, cur_size, ratio);
}

static void
ensure_body_size(YogEnv* env, YogVal array, uint_t size)
{
    SAVE_ARG(env, array);

    YogVal old_body = YUNDEF;
    PUSH_LOCAL(env, old_body);

    YogVal body = PTR_AS(YogArray, array)->body;
    if (PTR_AS(YogValArray, body)->size < size) {
        old_body = body;
        size_t old_size = PTR_AS(YogValArray, old_body)->size;
#define INCREASE_RATIO  (2)
        uint_t new_size = get_next_size(size, old_size, INCREASE_RATIO);
#undef INCREASE_RATIO
        YogVal new_body = YogValArray_new(env, new_size);
        size_t cur_size = PTR_AS(YogArray, array)->size;
        YogVal* to = PTR_AS(YogValArray, new_body)->items;
        YogVal* from = PTR_AS(YogValArray, old_body)->items;
        memcpy(to, from, sizeof(YogVal) * cur_size);

        YogGC_UPDATE_PTR(env, PTR_AS(YogArray, array), body, new_body);
    }

    RETURN_VOID(env);
}

void
YogArray_push(YogEnv* env, YogVal array, YogVal val)
{
    SAVE_ARGS2(env, array, val);

    ensure_body_size(env, array, YogArray_size(env, array) + 1);

    YogVal body = PTR_AS(YogArray, array)->body;
    size_t size = PTR_AS(YogArray, array)->size;
    YogGC_UPDATE_PTR(env, PTR_AS(YogValArray, body), items[size], val);
    PTR_AS(YogArray, array)->size++;

    RETURN_VOID(env);
}

void
YogArray_extend(YogEnv* env, YogVal array, YogVal a)
{
    SAVE_ARGS2(env, array, a);

    uint_t old_size = YogArray_size(env, array);
    uint_t new_size = old_size + YogArray_size(env, a);
    ensure_body_size(env, array, new_size);

    YogVal to = PTR_AS(YogArray, array)->body;
    YogVal* p = &PTR_AS(YogValArray, to)->items[old_size];
    YogVal from = PTR_AS(YogArray, a)->body;
    YogVal* q = &PTR_AS(YogValArray, from)->items[0];
    memcpy(p, q, sizeof(YogVal) * YogArray_size(env, a));

    PTR_AS(YogArray, array)->size = new_size;

    RETURN_VOID(env);
}

static void
YogArray_keep_children(YogEnv* env, void* ptr, ObjectKeeper keeper, void* heap)
{
    YogBasicObj_keep_children(env, ptr, keeper, heap);

    YogArray* array = PTR_AS(YogArray, ptr);
    YogGC_KEEP(env, array, body, keeper, heap);
}

static YogVal
alloc_obj(YogEnv* env, YogVal klass, uint_t size)
{
    SAVE_ARG(env, klass);

    YogVal body = YUNDEF;
    YogVal array = YUNDEF;
    PUSH_LOCALS2(env, body, array);

    body = YogValArray_new(env, size);
    array = ALLOC_OBJ(env, YogArray_keep_children, NULL, YogArray);
    YogBasicObj_init(env, array, TYPE_ARRAY, 0, klass);
    PTR_AS(YogArray, array)->size = 0;
    YogGC_UPDATE_PTR(env, PTR_AS(YogArray, array), body, body);

    RETURN(env, array);
}

YogVal
YogArray_of_size(YogEnv* env, uint_t size)
{
    return alloc_obj(env, env->vm->cArray, size);
}

static YogVal
alloc(YogEnv* env, YogVal klass)
{
    SAVE_ARG(env, klass);

    YogVal array = YUNDEF;
    PUSH_LOCAL(env, array);

#define INIT_SIZE   1
    array = alloc_obj(env, klass, INIT_SIZE);
#undef INIT_SIZE

    RETURN(env, array);
}

YogVal
YogArray_new(YogEnv* env)
{
#define INIT_SIZE   (1)
    return YogArray_of_size(env, INIT_SIZE);
#undef INIT_SIZE
}

YogVal
YogArray_add(YogEnv* env, YogVal self, YogVal array)
{
    SAVE_ARGS2(env, self, array);
    YogVal val = YUNDEF;
    PUSH_LOCAL(env, val);

    uint_t size = YogArray_size(env, array);
    uint_t i;
    for (i = 0; i < size; i++) {
        val = YogArray_at(env, array, i);
        YogArray_push(env, self, val);
    }

    RETURN(env, self);
}

static YogVal
add(YogEnv* env, YogVal self, YogVal pkg, YogVal args, YogVal kw, YogVal block)
{
    SAVE_ARGS5(env, self, pkg, args, kw, block);
    YogVal array = YUNDEF;
    YogVal right = YUNDEF;
    PUSH_LOCALS2(env, array, right);

    YogCArg params[] = { { "a", &right }, { NULL, NULL } };
    YogGetArgs_parse_args(env, "+", params, args, kw);
    CHECK_SELF_TYPE(env, self);
    if (!IS_PTR(right) || (BASIC_OBJ_TYPE(right) != TYPE_ARRAY)) {
        YogError_raise_TypeError(env, "operand must be Array");
    }

    array = YogArray_new(env);
    YogArray_add(env, array, self);
    YogArray_add(env, array, right);

    RETURN(env, array);
}

static YogVal
lshift(YogEnv* env, YogVal self, YogVal pkg, YogVal args, YogVal kw, YogVal block)
{
    SAVE_ARGS5(env, self, pkg, args, kw, block);
    YogVal elem = YUNDEF;
    PUSH_LOCAL(env, elem);

    YogCArg params[] = { { "obj", &elem }, { NULL, NULL } };
    YogGetArgs_parse_args(env, "<<", params, args, kw);
    CHECK_SELF_TYPE(env, self);

    YogArray_push(env, self, elem);

    RETURN(env, self);
}

static int_t
normalize_index(YogEnv* env, YogVal self, int_t index)
{
    return 0 <= index ? index : YogArray_size(env, self) + index;
}

static void
assign(YogEnv* env, YogVal self, int_t index, YogVal val)
{
    uint_t size = YogArray_size(env, self);
    int_t n = normalize_index(env, self, index);
    if ((n < 0) || (size <= n)) {
        const char* fmt = "Array assignment index (%d) out of range (%u)";
        YogError_raise_IndexError(env, fmt, index, size);
    }
    YogVal body = PTR_AS(YogArray, self)->body;
    YogGC_UPDATE_PTR(env, PTR_AS(YogValArray, body), items[n], val);
}

static YogVal
assign_subscript(YogEnv* env, YogVal self, YogVal pkg, YogVal args, YogVal kw, YogVal block)
{
    SAVE_ARGS5(env, self, pkg, args, kw, block);
    YogVal index = YUNDEF;
    YogVal val = YUNDEF;
    YogVal body = YUNDEF;
    PUSH_LOCALS3(env, index, val, body);
    YogCArg params[] = {
        { "index", &index }, { "value", &val}, { NULL, NULL } };
    YogGetArgs_parse_args(env, "[]=", params, args, kw);
    CHECK_SELF_TYPE(env, self);
    if (!IS_FIXNUM(index)) {
        YogError_raise_TypeError(env, "index must be Fixnum");
    }

    assign(env, self, VAL2INT(index), val);

    RETURN(env, self);
}

YogVal
YogArray_subscript(YogEnv* env, YogVal self, YogVal index)
{
    if (!IS_FIXNUM(index)) {
        YogError_raise_TypeError(env, "Index must be Fixnum");
    }

    int_t n = VAL2INT(index);
    uint_t size = YogArray_size(env, self);
    if (n < 0) {
        n += size;
    }
    if ((n < 0) || (size <= n)) {
        YogError_raise_IndexError(env, "Array index out of range");
    }
    return YogArray_at(env, self, n);
}

static YogVal
init(YogEnv* env, YogHandle* self, YogHandle* pkg, YogHandle* size, YogHandle* block)
{
    CHECK_SELF_TYPE2(env, self);
    YogMisc_check_Fixnum(env, size, "size");
    if (VAL2INT(HDL2VAL(size)) < 0) {
        const char* fmt = "size must be positive, not %d";
        YogError_raise_ValueError(env, fmt, VAL2INT(HDL2VAL(size)));
    }
    YogVal body = YogValArray_new(env, VAL2INT(HDL2VAL(size)));
    YogGC_UPDATE_PTR(env, HDL_AS(YogArray, self), body, body);
    HDL_AS(YogArray, self)->size = VAL2INT(HDL2VAL(size));
    if ((block == NULL) || IS_NIL(HDL2VAL(block))) {
        return HDL2VAL(self);
    }
    int_t i;
    for (i = 0; i < VAL2INT(HDL2VAL(size)); i++) {
        YogVal val = YogCallable_call1(env, HDL2VAL(block), INT2VAL(i));
        assign(env, HDL2VAL(self), i, val);
    }

    return HDL2VAL(self);
}

static YogVal
subscript(YogEnv* env, YogHandle* self, YogHandle* pkg, YogHandle* index)
{
    CHECK_SELF_TYPE2(env, self);
    return YogArray_subscript(env, HDL2VAL(self), HDL2VAL(index));
}

static YogVal
get(YogEnv* env, YogVal self, YogVal pkg, YogVal args, YogVal kw, YogVal block)
{
    SAVE_ARGS5(env, self, pkg, args, kw, block);
    YogVal val = YUNDEF;
    YogVal index = YUNDEF;
    YogVal default_ = YNIL;
    PUSH_LOCALS3(env, val, index, default_);
    CHECK_SELF_TYPE(env, self);
    YogCArg params[] = {
        { "index", &index },
        { "|", NULL },
        { "default", &default_ },
        { NULL, NULL } };
    YogGetArgs_parse_args(env, "get", params, args, kw);

    uint_t size = YogArray_size(env, self);
    int_t n = VAL2INT(index);
    if ((n < 0) || (size <= n)) {
        RETURN(env, default_);
    }

    val = YogArray_at(env, self, n);
    RETURN(env, val);
}

static YogVal
each(YogEnv* env, YogVal self, YogVal pkg, YogVal args, YogVal kw, YogVal block)
{
    SAVE_ARGS5(env, self, pkg, args, kw, block);
    YogVal arg[] = { YUNDEF };
    PUSH_LOCALSX(env, array_sizeof(arg), arg);

    YogCArg params[] = { { NULL, NULL } };
    YogGetArgs_parse_args(env, "each", params, args, kw);

    uint_t size = YogArray_size(env, self);
    uint_t i;
    for (i = 0; i < size; i++) {
        arg[0] = YogArray_at(env, self, i);
        YogCallable_call(env, block, array_sizeof(arg), arg);
    }

    RETURN(env, self);
}

static YogVal
get_empty(YogEnv* env, YogVal self, YogVal pkg, YogVal args, YogVal kw, YogVal block)
{
    SAVE_ARGS5(env, self, pkg, args, kw, block);
    CHECK_SELF_TYPE(env, self);
    YogCArg params[] = { { NULL, NULL } };
    YogGetArgs_parse_args(env, "get_empty", params, args, kw);

    if (YogArray_size(env, self) < 1) {
        RETURN(env, YTRUE);
    }

    RETURN(env, YFALSE);
}

static YogVal
get_size(YogEnv* env, YogVal self, YogVal pkg, YogVal args, YogVal kw, YogVal block)
{
    SAVE_ARGS5(env, self, pkg, args, kw, block);
    YogVal retval = YUNDEF;
    PUSH_LOCAL(env, retval);

    YogCArg params[] = { { NULL, NULL } };
    YogGetArgs_parse_args(env, "get_size", params, args, kw);

    uint_t size = YogArray_size(env, self);
    /* TODO: cast causes overflow */
    retval = INT2VAL(size);

    RETURN(env, retval);
}

YogVal
YogArray_shift(YogEnv* env, YogVal self)
{
    SAVE_ARG(env, self);
    YogVal retval = YUNDEF;
    YogVal body = YUNDEF;
    YogVal elem = YUNDEF;
    PUSH_LOCALS3(env, retval, body, elem);

    uint_t size = YogArray_size(env, self);
    YOG_ASSERT(env, 0 < size, "array is empty");

    retval = YogArray_at(env, self, 0);

    body = PTR_AS(YogArray, self)->body;
    uint_t i;
    for (i = 1; i < size; i++) {
        elem = YogArray_at(env, self, i);
        YogGC_UPDATE_PTR(env, PTR_AS(YogValArray, body), items[i - 1], elem);
    }
    PTR_AS(YogValArray, body)->items[size - 1] = YUNDEF;

    PTR_AS(YogArray, self)->size--;

    RETURN(env, retval);
}

YogVal
YogArray_pop(YogEnv* env, YogVal self)
{
    SAVE_ARG(env, self);
    YogVal retval = YUNDEF;
    PUSH_LOCAL(env, retval);

    uint_t size = YogArray_size(env, self);
    if (size < 1) {
        YogError_raise_IndexError(env, "pop from empty list");
    }

    retval = YogArray_at(env, self, size - 1);
    PTR_AS(YogArray, self)->size--;

    RETURN(env, retval);
}

static YogVal
pop(YogEnv* env, YogVal self, YogVal pkg, YogVal args, YogVal kw, YogVal block)
{
    SAVE_ARGS5(env, self, pkg, args, kw, block);
    YogVal obj = YUNDEF;
    PUSH_LOCAL(env, obj);

    YogCArg params[] = { { NULL, NULL } };
    YogGetArgs_parse_args(env, "pop", params, args, kw);
    CHECK_SELF_TYPE(env, self);

    obj = YogArray_pop(env, self);

    RETURN(env, obj);
}

static YogVal
unshift(YogEnv* env, YogVal self, YogVal pkg, YogVal args, YogVal kw, YogVal block)
{
    SAVE_ARGS5(env, self, pkg, args, kw, block);
    YogVal obj = YUNDEF;
    YogVal val = YUNDEF;
    PUSH_LOCALS2(env, obj, val);
    CHECK_SELF_TYPE(env, self);
    YogCArg params[] = { { "obj", &obj }, { NULL, NULL } };
    YogGetArgs_parse_args(env, "unshift", params, args, kw);

    uint_t size = YogArray_size(env, self);
    ensure_body_size(env, self, size + 1);

    YogVal body = PTR_AS(YogArray, self)->body;
    uint_t i;
    for (i = size; 0 < i; i--) {
        val = YogArray_at(env, self, i - 1);
        YogGC_UPDATE_PTR(env, PTR_AS(YogValArray, body), items[i], val);
    }
    YogGC_UPDATE_PTR(env, PTR_AS(YogValArray, body), items[0], obj);
    PTR_AS(YogArray, self)->size++;

    RETURN(env, self);
}

static YogVal
shift(YogEnv* env, YogVal self, YogVal pkg, YogVal args, YogVal kw, YogVal block)
{
    SAVE_ARGS5(env, self, pkg, args, kw, block);
    YogVal obj = YUNDEF;
    PUSH_LOCAL(env, obj);
    CHECK_SELF_TYPE(env, self);
    YogCArg params[] = { { NULL, NULL } };
    YogGetArgs_parse_args(env, "shift", params, args, kw);
    if (YogArray_size(env, self) < 1) {
        YogError_raise_IndexError(env, "shift from empty array");
    }

    obj = YogArray_shift(env, self);

    RETURN(env, obj);
}

static YogVal
push(YogEnv* env, YogVal self, YogVal pkg, YogVal args, YogVal kw, YogVal block)
{
    SAVE_ARGS5(env, self, pkg, args, kw, block);
    YogVal obj = YUNDEF;
    PUSH_LOCAL(env, obj);

    YogCArg params[] = { { "obj", &obj}, { NULL, NULL } };
    YogGetArgs_parse_args(env, "push", params, args, kw);
    CHECK_SELF_TYPE(env, self);

    YogArray_push(env, self, obj);

    RETURN(env, self);
}

void
YogArray_eval_builtin_script(YogEnv* env, YogVal klass)
{
#if !defined(MINIYOG)
    const char* src =
#   include "array.inc"
    ;
    YogMisc_eval_source(env, VAL2HDL(env, klass), src);
#endif
}

void
YogArray_define_classes(YogEnv* env, YogVal pkg)
{
    SAVE_ARG(env, pkg);
    YogVal cArray = YUNDEF;
    PUSH_LOCAL(env, cArray);
    YogVM* vm = env->vm;

    cArray = YogClass_new(env, "Array", vm->cObject);
    YogClass_define_allocator(env, cArray, alloc);
#define DEFINE_METHOD(name, f)  do { \
    YogClass_define_method(env, cArray, pkg, (name), (f)); \
} while (0)
    DEFINE_METHOD("+", add);
    DEFINE_METHOD("<<", lshift);
    DEFINE_METHOD("[]=", assign_subscript);
    DEFINE_METHOD("each", each);
    DEFINE_METHOD("get", get);
    DEFINE_METHOD("pop", pop);
    DEFINE_METHOD("push", push);
    DEFINE_METHOD("shift", shift);
    DEFINE_METHOD("unshift", unshift);
#undef DEFINE_METHOD
#define DEFINE_METHOD2(name, ...) do { \
    YogClass_define_method2(env, cArray, pkg, (name), __VA_ARGS__); \
} while (0)
    DEFINE_METHOD2("[]", subscript, "index", NULL);
    DEFINE_METHOD2("init", init, "size", "|", "&", NULL);
#undef DEFINE_METHOD2
#define DEFINE_PROP(name, getter, setter)   do { \
    YogClass_define_property(env, cArray, pkg, (name), (getter), (setter)); \
} while (0)
    DEFINE_PROP("empty?", get_empty, NULL);
    DEFINE_PROP("size", get_size, NULL);
#undef DEFINE_PROP
    vm->cArray = cArray;

    RETURN_VOID(env);
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
