#if !defined(__YOG_ARRAY_H__)
#define __YOG_ARRAY_H__

#include "yog/object.h"
#include "yog/yog.h"

struct YogValArray {
    uint_t size;
    YogVal items[0];
};

typedef struct YogValArray YogValArray;

struct YogArray {
    YOGBASICOBJ_HEAD;
    uint_t size;
    YogVal body;
};

typedef struct YogArray YogArray;

DECL_AS_TYPE(YogArray_new);
#define TYPE_ARRAY  TO_TYPE(YogArray_new)

/* PROTOTYPE_START */

/**
 * DON'T EDIT THIS AREA. HERE IS GENERATED BY update_prototype.py.
 */
/* src/array.c */
YOG_EXPORT YogVal YogArray_add(YogEnv*, YogVal, YogVal);
YOG_EXPORT YogVal YogArray_at(YogEnv*, YogVal, uint_t);
YOG_EXPORT void YogArray_define_classes(YogEnv*, YogVal);
YOG_EXPORT void YogArray_eval_builtin_script(YogEnv*, YogVal);
YOG_EXPORT void YogArray_extend(YogEnv*, YogVal, YogVal);
YOG_EXPORT YogVal YogArray_new(YogEnv*);
YOG_EXPORT YogVal YogArray_of_size(YogEnv*, uint_t);
YOG_EXPORT void YogArray_push(YogEnv*, YogVal, YogVal);
YOG_EXPORT YogVal YogArray_shift(YogEnv*, YogVal);
YOG_EXPORT uint_t YogArray_size(YogEnv*, YogVal);
YOG_EXPORT YogVal YogArray_subscript(YogEnv*, YogVal, YogVal);
YOG_EXPORT YogVal YogValArray_at(YogEnv*, YogVal, uint_t);
YOG_EXPORT YogVal YogValArray_new(YogEnv*, uint_t);
YOG_EXPORT uint_t YogValArray_size(YogEnv*, YogVal);

/* PROTOTYPE_END */

#endif
/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
