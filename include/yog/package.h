#if !defined(__YOG_PACKAGE_H__)
#define __YOG_PACKAGE_H__

#include "yog/object.h"
#include "yog/yog.h"

struct YogPackage {
    YOGOBJ_HEAD;
    YogVal code;
};

typedef struct YogPackage YogPackage;

#include "yog/klass.h"

/* PROTOTYPE_START */

/**
 * DON'T EDIT THIS AREA. HERE IS GENERATED BY update_prototype.py.
 */

/* src/package.c */
void YogPackage_define_method(YogEnv*, YogVal, const char*, void*, unsigned int, unsigned int, unsigned int, unsigned int, ...);
YogVal YogPackage_klass_new(YogEnv*);
YogVal YogPackage_new(YogEnv*);

/* PROTOTYPE_END */

#endif
/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
