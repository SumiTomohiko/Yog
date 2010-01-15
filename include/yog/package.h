#if !defined(__YOG_PACKAGE_H__)
#define __YOG_PACKAGE_H__

#include "yog/object.h"
#include "yog/yog.h"

struct YogPackage {
    YOGOBJ_HEAD;
};

typedef struct YogPackage YogPackage;

#define TYPE_PACKAGE    ((type_t)YogPackage_new)

#include "yog/class.h"

/* PROTOTYPE_START */

/**
 * DON'T EDIT THIS AREA. HERE IS GENERATED BY update_prototype.py.
 */
#if defined(__cplusplus)
extern "C" {
#endif
/* src/package.c */
void YogPackage_define_classes(YogEnv*, YogVal);
void YogPackage_define_function(YogEnv*, YogVal, const char*, void*);
void YogPackage_init(YogEnv*, YogVal, type_t);
void YogPackage_keep_children(YogEnv*, void*, ObjectKeeper, void*);
YogVal YogPackage_new(YogEnv*);


#if defined(__cplusplus)
}
#endif
/* PROTOTYPE_END */

#endif
/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
