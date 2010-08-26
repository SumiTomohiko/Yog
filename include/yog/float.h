#if !defined(YOG_FLOAT_H_INCLUDED)
#define YOG_FLOAT_H_INCLUDED

#include "yog/object.h"
#include "yog/yog.h"

struct YogFloat {
    YOGBASICOBJ_HEAD;
    double val;
};

typedef struct YogFloat YogFloat;

DECL_AS_TYPE(YogFloat_new);
#define TYPE_FLOAT TO_TYPE(YogFloat_new)

#define FLOAT_NUM(f)    PTR_AS(YogFloat, (f))->val

/* PROTOTYPE_START */

/**
 * DON'T EDIT THIS AREA. HERE IS GENERATED BY update_prototype.py.
 */
/* src/float.c */
YOG_EXPORT YogVal YogFloat_binop_add(YogEnv*, YogHandle*, YogHandle*);
YOG_EXPORT YogVal YogFloat_binop_divide(YogEnv*, YogHandle*, YogHandle*);
YOG_EXPORT YogVal YogFloat_binop_floor_divide(YogEnv*, YogHandle*, YogHandle*);
YOG_EXPORT YogVal YogFloat_binop_multiply(YogEnv*, YogHandle*, YogHandle*);
YOG_EXPORT YogVal YogFloat_binop_power(YogEnv*, YogHandle*, YogHandle*);
YOG_EXPORT YogVal YogFloat_binop_subtract(YogEnv*, YogHandle*, YogHandle*);
YOG_EXPORT YogVal YogFloat_binop_ufo(YogEnv*, YogVal, YogVal);
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
