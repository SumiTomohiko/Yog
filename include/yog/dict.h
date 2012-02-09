#if !defined(YOG_DICT_H_INCLUDED)
#define YOG_DICT_H_INCLUDED

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
YogVal YogDictIterator_current_key(YogEnv*, YogVal);
YogVal YogDictIterator_current_value(YogEnv*, YogVal);
BOOL YogDictIterator_next(YogEnv*, YogVal);
YogVal YogDict_add(YogEnv*, YogVal, YogVal);
YogVal YogDict_alloc(YogEnv*, YogVal);
void YogDict_define_classes(YogEnv*, YogVal);
void YogDict_eval_builtin_script(YogEnv*, YogVal);
YogVal YogDict_get(YogEnv*, YogVal, YogVal);
YogVal YogDict_get_iterator(YogEnv*, YogVal);
BOOL YogDict_include(YogEnv*, YogVal, YogVal);
YogVal YogDict_new(YogEnv*);
void YogDict_set(YogEnv*, YogVal, YogVal, YogVal);
uint_t YogDict_size(YogEnv*, YogVal);
YogVal YogDict_subscript(YogEnv*, YogVal, YogVal);

/* PROTOTYPE_END */

#endif
/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */

