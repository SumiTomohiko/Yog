#if !defined(__YOG_FLOAT_H__)
#define __YOG_FLOAT_H__

#include "yog/object.h"
#include "yog/yog.h"

struct YogFloat {
    YOGBASICOBJ_HEAD;
    double val;
};

typedef struct YogFloat YogFloat;

#define TYPE_FLOAT  ((type_t)YogFloat_new)

#define FLOAT_NUM(f)    PTR_AS(YogFloat, (f))->val

/* PROTOTYPE_START */

/**
 * DON'T EDIT THIS AREA. HERE IS GENERATED BY update_prototype.py.
 */
/* src/float.c */
YOG_EXPORT void YogFloat_define_classes(YogEnv*, YogVal);
YOG_EXPORT YogVal YogFloat_from_float(YogEnv*, double);
YOG_EXPORT YogVal YogFloat_from_str(YogEnv*, YogVal);
YOG_EXPORT YogVal YogFloat_new(YogEnv*);
YOG_EXPORT YogVal YogFloat_power(YogEnv*, YogVal, int_t);

/* PROTOTYPE_END */

#endif
/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
