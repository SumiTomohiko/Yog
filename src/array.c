#include <string.h>
#include "yog/array.h"
#include "yog/error.h"
#include "yog/gc.h"
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

static void 
ensure_body_size(YogEnv* env, YogVal array, unsigned int size) 
{
    SAVE_ARG(env, array);

    YogVal old_body = YUNDEF;
    PUSH_LOCAL(env, old_body);

    YogVal body = PTR_AS(YogArray, array)->body;
    if (PTR_AS(YogValArray, body)->size < size) {
        old_body = body;
#define INCREASE_RATIO  (2)
        size_t old_size = PTR_AS(YogValArray, old_body)->size;
        unsigned int new_size = INCREASE_RATIO * old_size;
        while (new_size < size) {
            new_size *= INCREASE_RATIO;
        }
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
    YogArray* array = ptr;
    YogGC_keep(env, &array->body, keeper, heap);
}

YogVal
YogArray_of_size(YogEnv* env, unsigned int size)
{
    SAVE_LOCALS(env);

    YogVal body = YUNDEF;
    YogVal array = YUNDEF;
    PUSH_LOCALS2(env, body, array);

    body = YogValArray_new(env, size);
    array = ALLOC_OBJ(env, YogArray_keep_children, NULL, YogArray);
    PTR_AS(YogArray, array)->size = 0;
    PTR_AS(YogArray, array)->body = body;

    RETURN(env, array);
}

YogVal 
YogArray_new(YogEnv* env)
{
#define INIT_SIZE   (1)
    return YogArray_of_size(env, INIT_SIZE);
#undef INIT_SIZE
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
