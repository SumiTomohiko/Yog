#include <string.h>
#include "yog/array.h"
#include "yog/yog.h"

unsigned int 
YogValArray_size(YogEnv* env, YogValArray* array) 
{
    return array->size;
}

YogVal 
YogValArray_at(YogEnv* env, YogValArray* array, unsigned int n) 
{
    YOG_ASSERT(env, n < array->size, "Index exceed array body size.");
    return array->items[n];
}

YogVal 
YogArray_at(YogEnv* env, YogArray* array, unsigned int n) 
{
    YOG_ASSERT(env, n < array->size, "Index exceed array size.");
    return YogValArray_at(env, array->body, n);
}

unsigned int 
YogArray_size(YogEnv* env, YogArray* array) 
{
    return array->size;
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

YogValArray* 
YogValArray_new(YogEnv* env, unsigned int size) 
{
    YogValArray* array = ALLOC_OBJ_ITEM(env, YogValArray_keep_children, YogValArray, size, YogVal);
    array->size = size;
    unsigned int i = 0;
    for (i = 0; i < size; i++) {
        array->items[i] = YUNDEF;
    }

    return array;
}

static void 
ensure_body_size(YogEnv* env, YogArray* array, unsigned int size) 
{
    if (array->body->size < size) {
        YogValArray* old_body = array->body;
#define INCREASE_RATIO  (2)
        unsigned int new_size = INCREASE_RATIO * old_body->size;
        while (new_size < size) {
            new_size *= INCREASE_RATIO;
        }
#undef INCREASE_RATIO
        YogValArray* new_body = YogValArray_new(env, new_size);
        memcpy(new_body->items, old_body->items, sizeof(YogVal) * array->size);

        array->body = new_body;
    }
}

void 
YogArray_push(YogEnv* env, YogArray* array, YogVal val) 
{
    ensure_body_size(env, array, YogArray_size(env, array) + 1);

    array->body->items[array->size] = val;
    array->size++;
}

void 
YogArray_extend(YogEnv* env, YogArray* array, YogArray* a) 
{
    unsigned int old_size = YogArray_size(env, array);
    unsigned int new_size = old_size + YogArray_size(env, a);
    ensure_body_size(env, array, new_size);
    memcpy(&array->body->items[old_size], &a->body->items[0], sizeof(YogVal) * YogArray_size(env, a));
    array->size = new_size;
}

static void 
YogArray_keep_children(YogEnv* env, void* ptr, ObjectKeeper keeper)
{
    YogArray* array = ptr;
    array->body = (*keeper)(env, array->body);
}

YogArray* 
YogArray_new(YogEnv* env)
{
#define INIT_SIZE   (1)
    YogValArray* body = YogValArray_new(env, INIT_SIZE);
#undef INIT_SIZE

    YogArray* array = ALLOC_OBJ(env, YogArray_keep_children, YogArray);
    array->size = 0;
    array->body = body;

    return array;
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
