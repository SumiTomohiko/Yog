#include <string.h>
#include "yog/yog.h"

void 
YogValArray_push(YogEnv* env, YogValArray* array, YogVal val) 
{
    Yog_assert(env, array->size + 1 <= array->capacity, "Can't push.");
    array->items[array->size] = val;
    array->size++;
}

YogVal 
YogValArray_pop(YogEnv* env, YogValArray* array) 
{
    Yog_assert(env, 0 < array->size, "Can't pop.");
    YogVal val = array->items[array->size - 1];
    array->size--;
    return val;
}

YogVal 
YogValArray_at(YogEnv* env, YogValArray* array, unsigned int n) 
{
    Yog_assert(env, n < array->size, "Index exceed array size.");
    return array->items[n];
}

YogVal 
YogArray_at(YogEnv* env, YogArray* array, unsigned int n) 
{
    return YogValArray_at(env, array->body, n);
}

unsigned int 
YogValArray_size(YogEnv* env, YogValArray* array) 
{
    return array->size;
}

unsigned int 
YogArray_size(YogEnv* env, YogArray* array) 
{
    return YogValArray_size(env, array->body);
}

static void 
gc_valarray_children(YogEnv* env, void* ptr, DoGc do_gc) 
{
    YogValArray* array = ptr;
    unsigned int size = array->size;
    unsigned int i = 0;
    for (i = 0; i < size; i++) {
        YogVal val = array->items[i];
        if (IS_PTR(val)) {
            YOGVAL_PTR(array->items[i]) = do_gc(env, YOGVAL_PTR(val));
        }
    }
}

YogValArray* 
YogValArray_new(YogEnv* env, unsigned int capacity) 
{
    YogValArray* array = ALLOC_OBJ_ITEM(env, gc_valarray_children, YogValArray, capacity, YogVal);
    array->size = 0;
    array->capacity = capacity;

    return array;
}

void 
YogArray_push(YogEnv* env, YogArray* array, YogVal val) 
{
    if (array->body->capacity < array->body->size + 1) {
        YogValArray* old_body = array->body;
#define INCREASE_RATIO  (2)
        unsigned int new_capacity = INCREASE_RATIO * old_body->size;
#undef INCREASE_RATIO
        YogValArray* new_body = YogValArray_new(env, new_capacity);
        memcpy(new_body->items, old_body->items, sizeof(YogVal) * old_body->size);
        new_body->size = old_body->size;

        array->body = new_body;
    }

    YogValArray_push(env, array->body, val);
}

static void 
gc_array_children(YogEnv* env, void* ptr, DoGc do_gc) 
{
    YogArray* array = ptr;
    array->body = do_gc(env, array->body);
}

YogArray* 
YogArray_new(YogEnv* env)
{
#define INIT_SIZE   (1)
    YogValArray* body = YogValArray_new(env, INIT_SIZE);
#undef INIT_SIZE

    YogArray* array = ALLOC_OBJ(env, gc_array_children, YogArray);
    array->body = body;

    return array;
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
