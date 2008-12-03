#ifndef __YOG_KLASS_H__
#define __YOG_KLASS_H__

#include "yog/yog.h"

struct YogKlass {
    YOGOBJ_HEAD;
    Allocator allocator;
    ID name;
    struct YogKlass* super;
};

typedef struct YogKlass YogKlass;

/* PROTOTYPE_START */

/**
 * DON'T EDIT THIS AREA. HERE IS GENERATED BY update_prototype.py.
 */

/* src/klass.c */
YogBasicObj* YogKlass_allocate(YogEnv*, YogKlass*);
void YogKlass_define_allocator(YogEnv*, YogKlass*, Allocator);
void YogKlass_define_method(YogEnv*, YogKlass*, const char*, void*, unsigned int, unsigned int, unsigned int, int, ...);
void YogKlass_klass_init(YogEnv*, YogKlass*);
YogKlass* YogKlass_new(YogEnv*, const char*, YogKlass*);

/* PROTOTYPE_END */

#endif
/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
