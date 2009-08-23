#include <string.h>
#include "yog/array.h"
#include "yog/error.h"
#include "yog/eval.h"
#include "yog/frame.h"
#include "yog/function.h"
#include "yog/gc.h"
#include "yog/klass.h"
#include "yog/misc.h"
#include "yog/object.h"
#include "yog/string.h"
#include "yog/thread.h"
#include "yog/yog.h"

uint_t
YogValArray_size(YogEnv* env, YogVal array)
{
    return PTR_AS(YogValArray, array)->size;
}

YogVal
YogValArray_at(YogEnv* env, YogVal array, uint_t n)
{
    uint_t size = PTR_AS(YogValArray, array)->size;
    YOG_ASSERT(env, n < size, "Index exceed array body size.");
    return PTR_AS(YogValArray, array)->items[n];
}

YogVal
YogArray_at(YogEnv* env, YogVal array, uint_t n)
{
    size_t size = PTR_AS(YogArray, array)->size;
    YOG_ASSERT(env, n < size, "Index exceed array size.");

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
    YogValArray* array = ptr;

    uint_t size = array->size;
    uint_t i = 0;
    for (i = 0; i < size; i++) {
        YogGC_keep(env, &array->items[i], keeper, heap);
    }
}

YogVal
YogValArray_new(YogEnv* env, uint_t size)
{
    YogVal array = ALLOC_OBJ_ITEM(env, YogValArray_keep_children, NULL, YogValArray, size, YogVal);
    PTR_AS(YogValArray, array)->size = size;
    uint_t i = 0;
    for (i = 0; i < size; i++) {
        PTR_AS(YogValArray, array)->items[i] = YUNDEF;
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

        PTR_AS(YogArray, array)->body = new_body;
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
    PTR_AS(YogValArray, body)->items[size] = val;
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

    YogArray* array = ptr;
    YogGC_keep(env, &array->body, keeper, heap);
}

static YogVal
allocate_object(YogEnv* env, YogVal klass, uint_t size)
{
    SAVE_ARG(env, klass);

    YogVal body = YUNDEF;
    YogVal array = YUNDEF;
    PUSH_LOCALS2(env, body, array);

    body = YogValArray_new(env, size);
    array = ALLOC_OBJ(env, YogArray_keep_children, NULL, YogArray);
    YogBasicObj_init(env, array, 0, klass);
    PTR_AS(YogArray, array)->size = 0;
    PTR_AS(YogArray, array)->body = body;

    RETURN(env, array);
}

YogVal
YogArray_of_size(YogEnv* env, uint_t size)
{
    return allocate_object(env, env->vm->cArray, size);
}

static YogVal
allocate(YogEnv* env, YogVal klass)
{
    SAVE_ARG(env, klass);

    YogVal array = YUNDEF;
    PUSH_LOCAL(env, array);

#define INIT_SIZE   1
    array = allocate_object(env, klass, INIT_SIZE);
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
add(YogEnv* env, YogVal self, YogVal args, YogVal kw, YogVal block)
{
    SAVE_ARGS4(env, self, args, kw, block);
    YogVal array = YUNDEF;
    YogVal right = YUNDEF;
    YogVal val = YUNDEF;
    PUSH_LOCALS3(env, array, right, val);

    right = YogArray_at(env, args, 0);
    YOG_ASSERT(env, IS_PTR(right), "operand is not Array");
    YOG_ASSERT(env, IS_OBJ_OF(env, right, cArray), "operand is not Array");

    array = YogArray_new(env);
    YogArray_add(env, array, self);
    YogArray_add(env, array, right);

    RETURN(env, array);
}

static YogVal
lshift(YogEnv* env, YogVal self, YogVal args, YogVal kw, YogVal block)
{
    SAVE_ARGS4(env, self, args, kw, block);
    YogVal elem = YogArray_at(env, args, 0);
    YogArray_push(env, self, elem);
    RETURN(env, self);
}

static YogVal
subscript(YogEnv* env, YogVal self, YogVal args, YogVal kw, YogVal block)
{
    SAVE_ARGS4(env, self, args, kw, block);
    YogVal index = YogArray_at(env, args, 0);
    YogVal v = YogArray_at(env, self, VAL2INT(index));
    RETURN(env, v);
}

static YogVal
each(YogEnv* env, YogVal self, YogVal args, YogVal kw, YogVal block)
{
    SAVE_ARGS4(env, self, args, kw, block);

    YogVal arg[] = { YUNDEF };
    PUSH_LOCALSX(env, 1, arg);

    uint_t size = YogArray_size(env, self);
    uint_t i;
    for (i = 0; i < size; i++) {
        arg[0] = YogArray_at(env, self, i);
        YogCallable_call(env, block, array_sizeof(arg), arg);
    }

    RETURN(env, self);
}

static YogVal
get_size(YogEnv* env, YogVal self, YogVal args, YogVal kw, YogVal block)
{
    SAVE_ARGS4(env, self, args, kw, block);
    YogVal retval = YUNDEF;
    PUSH_LOCAL(env, retval);

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
        PTR_AS(YogValArray, body)->items[i - 1] = elem;
    }
    PTR_AS(YogValArray, body)->items[size - 1] = YUNDEF;

    PTR_AS(YogArray, self)->size--;

    RETURN(env, retval);
}

static YogVal
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
pop(YogEnv* env, YogVal self, YogVal args, YogVal kw, YogVal block)
{
    SAVE_ARGS4(env, self, args, kw, block);
    YogVal obj = YUNDEF;
    PUSH_LOCAL(env, obj);

    obj = YogArray_pop(env, self);

    RETURN(env, obj);
}

static YogVal
push(YogEnv* env, YogVal self, YogVal args, YogVal kw, YogVal block)
{
    SAVE_ARGS4(env, self, args, kw, block);
    YogVal obj = YUNDEF;
    PUSH_LOCAL(env, obj);

    obj = YogArray_at(env, args, 0);
    YogArray_push(env, self, obj);

    RETURN(env, self);
}

YogVal
YogArray_klass_new(YogEnv* env)
{
    SAVE_LOCALS(env);

    YogVal klass = YUNDEF;
    PUSH_LOCAL(env, klass);

    klass = YogKlass_new(env, "Array", env->vm->cObject);
    YogKlass_define_allocator(env, klass, allocate);
#define DEFINE_METHOD(name, f)  YogKlass_define_method(env, klass, name, f)
    DEFINE_METHOD("+", add);
    DEFINE_METHOD("<<", lshift);
    DEFINE_METHOD("[]", subscript);
    DEFINE_METHOD("each", each);
    DEFINE_METHOD("pop", pop);
    DEFINE_METHOD("push", push);
#undef DEFINE_METHOD
    YogKlass_define_property(env, klass, "size", get_size, NULL);

    RETURN(env, klass);
}

void
YogArray_eval_builtin_script(YogEnv* env, YogVal cArray)
{
#if !defined(MINIYOG)
    const char* src =
#   include "array.inc"
    ;
    YogMisc_eval_source(env, cArray, src);
#endif
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
