#if !defined(YOG_MODULE_H_INCLUDED)
#define YOG_MODULE_H_INCLUDED

#include "yog/object.h"
#include "yog/yog.h"

struct YogModule {
    struct YogObj base;
    ID name;
};

typedef struct YogModule YogModule;

#define TYPE_MODULE TO_TYPE(YogModule_new)

/* PROTOTYPE_START */

/**
 * DON'T EDIT THIS AREA. HERE IS GENERATED BY update_prototype.py.
 */
/* src/module.c */
void YogModule_define_classes(YogEnv*, YogVal);
void YogModule_define_function(YogEnv*, YogVal, YogVal, const char*, YogAPI);
YogVal YogModule_new(YogEnv*, ID);
YogVal YogModule_of_name(YogEnv*, const char*);

/* PROTOTYPE_END */

#endif
/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
