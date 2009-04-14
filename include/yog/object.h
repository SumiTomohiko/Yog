#if !defined(__YOG_OBJECT_H__)
#define __YOG_OBJECT_H__

#include "yog/yog.h"

struct YogBasicObj {
    unsigned int flags;
    YogVal klass;
};

#define HAS_ATTRS   (1)

#define YOGBASICOBJ_HEAD    struct YogBasicObj base
#define YOGBASICOBJ(obj)    ((struct YogBasicObj*)obj)

struct YogObj {
    YOGBASICOBJ_HEAD;
    YogVal attrs;
};

#define YOGOBJ_HEAD struct YogObj base
#define YOGOBJ(obj) ((struct YogObj*)obj)

typedef struct YogObj YogObj;

typedef YogVal (*Allocator)(struct YogEnv*, YogVal);

#define KLASS_OF(v)     (PTR_AS(YogBasicObj, v)->klass)
#define IS_OBJ_OF(k, v) (IS_PTR(v) && (KLASS_OF(v) == env->vm->k))

/* PROTOTYPE_START */

/**
 * DON'T EDIT THIS AREA. HERE IS GENERATED BY update_prototype.py.
 */

/* src/object.c */
void YogBasicObj_init(YogEnv*, YogVal, unsigned int, YogVal);
void YogBasicObj_keep_children(YogEnv*, void*, ObjectKeeper);
YogVal YogObj_allocate(YogEnv*, YogVal);
YogVal YogObj_get_attr(YogEnv*, YogVal, ID);
void YogObj_init(YogEnv*, YogVal, unsigned int, YogVal);
void YogObj_keep_children(YogEnv*, void*, ObjectKeeper);
void YogObj_klass_init(YogEnv*, YogVal);
YogVal YogObj_new(YogEnv*, YogVal);
void YogObj_set_attr(YogEnv*, YogVal, const char*, YogVal);
void YogObj_set_attr_id(YogEnv*, YogVal, ID, YogVal);

/* PROTOTYPE_END */

#endif
/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
