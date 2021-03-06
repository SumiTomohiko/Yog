#if !defined(YOG_REGEXP_H_INCLUDED)
#define YOG_REGEXP_H_INCLUDED

#include "corgi.h"
#include "yog/object.h"
#include "yog/yog.h"

struct YogRegexp {
    YOGBASICOBJ_HEAD;
    CorgiRegexp* corgi_regexp;
};

typedef struct YogRegexp YogRegexp;

#define TYPE_REGEXP TO_TYPE(YogRegexp_new)

struct YogMatch {
    YOGBASICOBJ_HEAD;
    YogVal str;
    YogVal regexp;
    CorgiMatch corgi_match;
};

typedef struct YogMatch YogMatch;

#define TYPE_MATCH  ((type_t)YogMatch_new)

/* PROTOTYPE_START */

/**
 * DON'T EDIT THIS AREA. HERE IS GENERATED BY update_prototype.py.
 */
/* src/regexp.c */
YogVal YogMatch_new(YogEnv*, YogHandle*, YogHandle*);
YogVal YogRegexp_binop_search(YogEnv*, YogHandle*, YogHandle*);
void YogRegexp_define_classes(YogEnv*, YogVal);
YogVal YogRegexp_new(YogEnv*, YogVal, BOOL);

/* PROTOTYPE_END */

#endif
/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
