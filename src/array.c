#include <string.h>
#include "yog/array.h"
#include "yog/error.h"
#include "yog/thread.h"
#include "yog/vm.h"
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
    size_t size = OBJ_AS(YogArray, array)->size;
    YOG_ASSERT(env, n < size, "Index exceed array size.");

    return YogValArray_at(env, OBJ_AS(YogArray, array)->body, n);
}

unsigned int 
YogArray_size(YogEnv* env, YogVal array) 
{
    return OBJ_AS(YogArray, array)->size;
}

static void 
YogValArray_keep_children(YogEnv* env, void* ptr, ObjectKeeper keeper)
{
    YogValArray* array = ptr;

    unsigned int size = array->size;
    unsigned int i = 0;
    for (i = 0; i < size; i++) {
        array->items[i] = YogVal_keep(env, array->items[i], keeper);
    }
}

YogVal 
YogValArray_new(YogEnv* env, unsigned int size) 
{
    YogValArray* array = ALLOC_OBJ_ITEM(env, YogValArray_keep_children, NULL, YogValArray, size, YogVal);
    array->size = size;
    unsigned int i = 0;
    for (i = 0; i < size; i++) {
        array->items[i] = YUNDEF;
    }

    return PTR2VAL(array);
}

static void 
ensure_body_size(YogEnv* env, YogVal array, unsigned int size) 
{
    SAVE_ARG(env, array);

    YogVal old_body = YUNDEF;
    PUSH_LOCAL(env, old_body);

    YogVal body = OBJ_AS(YogArray, array)->body;
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
        size_t cur_size = OBJ_AS(YogArray, array)->size;
        YogVal* to = PTR_AS(YogValArray, new_body)->items;
        YogVal* from = PTR_AS(YogValArray, old_body)->items;
        memcpy(to, from, sizeof(YogVal) * cur_size);

        MODIFY(env, OBJ_AS(YogArray, array)->body, new_body);
    }

    RETURN_VOID(env);
}

void 
YogArray_push(YogEnv* env, YogVal array, YogVal val) 
{
    SAVE_ARGS2(env, array, val);

    ensure_body_size(env, array, YogArray_size(env, array) + 1);

    YogVal body = OBJ_AS(YogArray, array)->body;
    size_t size = OBJ_AS(YogArray, array)->size;
    MODIFY(env, PTR_AS(YogValArray, body)->items[size], val);
    OBJ_AS(YogArray, array)->size++;

    RETURN_VOID(env);
}

void 
YogArray_extend(YogEnv* env, YogVal array, YogVal a) 
{
    SAVE_ARGS2(env, array, a);

    unsigned int old_size = YogArray_size(env, array);
    unsigned int new_size = old_size + YogArray_size(env, a);
    ensure_body_size(env, array, new_size);

    YogVal to = OBJ_AS(YogArray, array)->body;
    YogVal* p = &PTR_AS(YogValArray, to)->items[old_size];
    YogVal from = OBJ_AS(YogArray, a)->body;
    YogVal* q = &PTR_AS(YogValArray, from)->items[0];
    memcpy(p, q, sizeof(YogVal) * YogArray_size(env, a));

    OBJ_AS(YogArray, array)->size = new_size;

    RETURN_VOID(env);
}

static void 
YogArray_keep_children(YogEnv* env, void* ptr, ObjectKeeper keeper)
{
    YogArray* array = ptr;
    array->body = YogVal_keep(env, array->body, keeper);
}

YogVal 
YogArray_new(YogEnv* env)
{
    SAVE_LOCALS(env);

#define INIT_SIZE   (1)
    YogVal body = YogValArray_new(env, INIT_SIZE);
#undef INIT_SIZE
    PUSH_LOCAL(env, body);

    YogVal array = OBJ2VAL(ALLOC_OBJ(env, YogArray_keep_children, NULL, YogArray));
    OBJ_AS(YogArray, array)->size = 0;
    MODIFY(env, OBJ_AS(YogArray, array)->body, body);

    RETURN(env, array);
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
