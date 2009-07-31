#if !defined(__YOG_KLASS_H__)
#define __YOG_KLASS_H__

#include <stdint.h>
#include "yog/object.h"
#include "yog/yog.h"

typedef YogVal (*AttrGetter)(YogEnv*, YogVal, ID);
typedef void (*Executor)(YogEnv*, YogVal, uint8_t, YogVal*, YogVal, uint8_t, YogVal*, YogVal, YogVal);
typedef YogVal (*Caller)(YogEnv*, YogVal, uint8_t, YogVal*, YogVal, uint8_t, YogVal*, YogVal, YogVal);

struct YogKlass {
    YOGOBJ_HEAD;
    Allocator allocator;
    ID name;
    YogVal super;
    AttrGetter get_attr;
    YogVal (*get_descr)(YogEnv*, YogVal, YogVal, YogVal);
    Executor exec;
    Caller call;
};

typedef struct YogKlass YogKlass;

/* PROTOTYPE_START */

/**
 * DON'T EDIT THIS AREA. HERE IS GENERATED BY update_prototype.py.
 */

/* src/klass.c */
YogVal YogKlass_allocate(YogEnv*, YogVal);
void YogKlass_define_allocator(YogEnv*, YogVal, Allocator);
void YogKlass_define_attr_getter(YogEnv*, YogVal, AttrGetter);
void YogKlass_define_caller(YogEnv*, YogVal, Caller);
void YogKlass_define_descr_getter(YogEnv*, YogVal, void*);
void YogKlass_define_executor(YogEnv*, YogVal, Executor);
void YogKlass_define_method(YogEnv*, YogVal, const char*, void*);
YogVal YogKlass_get_attr(YogEnv*, YogVal, ID);
void YogKlass_klass_init(YogEnv*, YogVal);
YogVal YogKlass_new(YogEnv*, const char*, YogVal);

/* PROTOTYPE_END */

#endif
/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
