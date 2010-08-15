#if !defined(__YOG_DICT_H__)
#define __YOG_DICT_H__

#include "yog/object.h"
#include "yog/yog.h"

struct YogDict {
    struct YogBasicObj base;
    YogVal tbl;
};

typedef struct YogDict YogDict;

DECL_AS_TYPE(YogDict_new);
#define TYPE_DICT TO_TYPE(YogDict_new)

/* PROTOTYPE_START */

/**
 * DON'T EDIT THIS AREA. HERE IS GENERATED BY update_prototype.py.
 */
/* src/dict.c */
YOG_EXPORT YogVal YogDictIterator_current_key(YogEnv*, YogVal);
YOG_EXPORT YogVal YogDictIterator_current_value(YogEnv*, YogVal);
YOG_EXPORT BOOL YogDictIterator_next(YogEnv*, YogVal);
YOG_EXPORT YogVal YogDict_add(YogEnv*, YogVal, YogVal);
YOG_EXPORT YogVal YogDict_alloc(YogEnv*, YogVal);
YOG_EXPORT void YogDict_define_classes(YogEnv*, YogVal);
YOG_EXPORT void YogDict_eval_builtin_script(YogEnv*, YogVal);
YOG_EXPORT YogVal YogDict_get(YogEnv*, YogVal, YogVal);
YOG_EXPORT YogVal YogDict_get_iterator(YogEnv*, YogVal);
YOG_EXPORT BOOL YogDict_include(YogEnv*, YogVal, YogVal);
YOG_EXPORT YogVal YogDict_new(YogEnv*);
YOG_EXPORT void YogDict_set(YogEnv*, YogVal, YogVal, YogVal);
YOG_EXPORT uint_t YogDict_size(YogEnv*, YogVal);
YOG_EXPORT YogVal YogDict_subscript(YogEnv*, YogVal, YogVal);

/* PROTOTYPE_END */

#endif
/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */

