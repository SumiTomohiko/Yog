#if !defined(YOG_REGEXP_H_INCLUDED)
#define YOG_REGEXP_H_INCLUDED

#include "oniguruma.h"
#include "yog/object.h"
#include "yog/yog.h"

struct YogRegexp {
    YOGBASICOBJ_HEAD;
    OnigRegex onig_regexp;
};

typedef struct YogRegexp YogRegexp;

DECL_AS_TYPE(YogRegexp_new);
#define TYPE_REGEXP TO_TYPE(YogRegexp_new)

struct YogMatch {
    YOGBASICOBJ_HEAD;
    YogVal str;
    YogVal regexp;
    OnigRegion* onig_region;
};

typedef struct YogMatch YogMatch;

#define TYPE_MATCH  ((type_t)YogMatch_new)

/* PROTOTYPE_START */

/**
 * DON'T EDIT THIS AREA. HERE IS GENERATED BY update_prototype.py.
 */
/* src/regexp.c */
YOG_EXPORT YogVal YogMatch_new(YogEnv*, YogVal, YogVal, OnigRegion*);
YOG_EXPORT YogVal YogRegexp_binop_match(YogEnv*, YogHandle*, YogHandle*);
YOG_EXPORT void YogRegexp_define_classes(YogEnv*, YogVal);
YOG_EXPORT YogVal YogRegexp_new(YogEnv*, YogVal, OnigOptionType);

/* PROTOTYPE_END */

#endif
/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
