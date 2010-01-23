#if !defined(__YOG_MODULE_H__)
#define __YOG_MODULE_H__

#include "yog/object.h"
#include "yog/yog.h"

struct YogModule {
    struct YogObj base;
    ID name;
};

typedef struct YogModule YogModule;

#define TYPE_MODULE     ((type_t)YogModule_new)

/* PROTOTYPE_START */

/**
 * DON'T EDIT THIS AREA. HERE IS GENERATED BY update_prototype.py.
 */
/* src/module.c */
void YogModule_define_classes(YogEnv*, YogVal);
void YogModule_define_function(YogEnv*, YogVal, YogVal, const char*, YogAPI);
YogVal YogModule_new(YogEnv*);

/* PROTOTYPE_END */

#endif
/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
