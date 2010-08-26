#if !defined(YOG_PACKAGE_H_INCLUDED)
#define YOG_PACKAGE_H_INCLUDED

#include "yog/object.h"
#include "yog/yog.h"

struct YogPackage {
    YOGOBJ_HEAD;
};

typedef struct YogPackage YogPackage;

DECL_AS_TYPE(YogPackage_new);
#define TYPE_PACKAGE TO_TYPE(YogPackage_new)

#include "yog/class.h"

/* PROTOTYPE_START */

/**
 * DON'T EDIT THIS AREA. HERE IS GENERATED BY update_prototype.py.
 */
/* src/package.c */
YOG_EXPORT void YogPackage_define_classes(YogEnv*, YogVal);
YOG_EXPORT void YogPackage_define_function(YogEnv*, YogVal, const char*, YogAPI);
YOG_EXPORT void YogPackage_define_function2(YogEnv*, YogHandle*, const char*, void*, ...);
YOG_EXPORT void YogPackage_init(YogEnv*, YogVal, type_t);
YOG_EXPORT void YogPackage_keep_children(YogEnv*, void*, ObjectKeeper, void*);
YOG_EXPORT YogVal YogPackage_new(YogEnv*);

/* PROTOTYPE_END */

#endif
/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
