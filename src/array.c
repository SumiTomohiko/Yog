#include <string.h>
#include "yog/array.h"
#include "yog/error.h"
#include "yog/eval.h"
#include "yog/frame.h"
#include "yog/gc.h"
#include "yog/klass.h"
#include "yog/object.h"
#include "yog/thread.h"
#include "yog/yog.h"

unsigned int 
YogValArray_size(YogEnv* env, YogVal array) 
{
    return PTR_AS(YogValArray, array)->size;
}

YogVal 
YogValArray_at(YogEnv* env, YogVal array, unsigned int n) 
{
    unsigned int size = PTR_AS(YogValArray, array)->size;
    YOG_ASSERT(env, n < size, "Index exceed array body size.");
    return PTR_AS(YogValArray, array)->items[n];
}

YogVal 
YogArray_at(YogEnv* env, YogVal array, unsigned int n) 
{
    size_t size = PTR_AS(YogArray, array)->size;
    YOG_ASSERT(env, n < size, "Index exceed array size.");

    return YogValArray_at(env, PTR_AS(YogArray, array)->body, n);
}

unsigned int 
YogArray_size(YogEnv* env, YogVal array) 
{
    return PTR_AS(YogArray, array)->size;
}

static void 
YogValArray_keep_children(YogEnv* env, void* ptr, ObjectKeeper keeper, void* heap)
{
    YogValArray* array = ptr;

    unsigned int size = array->size;
    unsigned int i = 0;
    for (i = 0; i < size; i++) {
        YogGC_keep(env, &array->items[i], keeper, heap);
    }
}

YogVal 
YogValArray_new(YogEnv* env, unsigned int size) 
{
    YogVal array = ALLOC_OBJ_ITEM(env, YogValArray_keep_children, NULL, YogValArray, size, YogVal);
    PTR_AS(YogValArray, array)->size = size;
    unsigned int i = 0;
    for (i = 0; i < size; i++) {
        PTR_AS(YogValArray, array)->items[i] = YUNDEF;
    }

    return array;
}

static unsigned int
multiple_size(unsigned int min_size, unsigned int cur_size, unsigned int ratio)
{
    while (cur_size < min_size) {
        cur_size *= ratio;
    }
    return cur_size;
}

static unsigned int
get_next_size(unsigned int min_size, unsigned int cur_size, unsigned int ratio)
{
    if (cur_size == 0) {
        return 1;
    }

    return multiple_size(min_size, cur_size, ratio);
}

static void 
ensure_body_size(YogEnv* env, YogVal array, unsigned int size) 
{
    SAVE_ARG(env, array);

    YogVal old_body = YUNDEF;
    PUSH_LOCAL(env, old_body);

    YogVal body = PTR_AS(YogArray, array)->body;
    if (PTR_AS(YogValArray, body)->size < size) {
        old_body = body;
        size_t old_size = PTR_AS(YogValArray, old_body)->size;
#define INCREASE_RATIO  (2)
        unsigned int new_size = get_next_size(size, old_size, INCREASE_RATIO);
#undef INCREASE_RATIO
        YogVal new_body = YogValArray_new(env, new_size);
        size_t cur_size = PTR_AS(YogArray, array)->size;
        YogVal* to = PTR_AS(YogValArray, new_body)->items;
        YogVal* from = PTR_AS(YogValArray, old_body)->items;
        memcpy(to, from, sizeof(YogVal) * cur_size);

        MODIFY(env, PTR_AS(YogArray, array)->body, new_body);
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
    MODIFY(env, PTR_AS(YogValArray, body)->items[size], val);
    PTR_AS(YogArray, array)->size++;

    RETURN_VOID(env);
}

void 
YogArray_extend(YogEnv* env, YogVal array, YogVal a) 
{
    SAVE_ARGS2(env, array, a);

    unsigned int old_size = YogArray_size(env, array);
    unsigned int new_size = old_size + YogArray_size(env, a);
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
allocate_object(YogEnv* env, YogVal klass, unsigned int size)
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
YogArray_of_size(YogEnv* env, unsigned int size)
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

    unsigned int size = YogArray_size(env, self);
    unsigned int i;
    for (i = 0; i < size; i++) {
        arg[0] = YogArray_at(env, self, i);
        YogEval_call_block(env, block, array_sizeof(arg), arg);
    }

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
    YogKlass_define_method(env, klass, "<<", lshift, 0, 0, 0, 0, "elem", NULL);
    YogKlass_define_method(env, klass, "[]", subscript, 0, 0, 0, 0, "n", NULL);
    YogKlass_define_method(env, klass, "each", each, 1, 0, 0, 0, "block", NULL);

    RETURN(env, klass);
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
