#if !defined(__YOG_PROPERTY_H__)
#define __YOG_PROPERTY_H__

#include "yog/object.h"
#include "yog/yog.h"

struct YogProperty {
    struct YogBasicObj base;

    YogVal getter;
    YogVal setter;
};

typedef struct YogProperty YogProperty;

#define TYPE_PROPERTY   ((type_t)YogProperty_new)

/* PROTOTYPE_START */

/**
 * DON'T EDIT THIS AREA. HERE IS GENERATED BY update_prototype.py.
 */
/* src/property.c */
YOG_EXPORT void YogProperty_define_classes(YogEnv*, YogVal);
YOG_EXPORT YogVal YogProperty_new(YogEnv*);

/* PROTOTYPE_END */

#endif
/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
