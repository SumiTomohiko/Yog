#if !defined(__YOG_REGEXP_H__)
#define __YOG_REGEXP_H__

#include "oniguruma.h"
#include "yog/object.h"
#include "yog/yog.h"

struct YogRegexp {
    YOGBASICOBJ_HEAD;
    OnigRegex onig_regexp;
};

typedef struct YogRegexp YogRegexp;

#define TYPE_REGEXP     ((type_t)YogRegexp_new)

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
#if defined(__cplusplus)
extern "C" {
#endif
/* src/regexp.c */
YogVal YogMatch_new(YogEnv*, YogVal, YogVal, OnigRegion*);
void YogRegexp_define_classes(YogEnv*, YogVal);
YogVal YogRegexp_new(YogEnv*, YogVal, OnigOptionType);


#if defined(__cplusplus)
}
#endif
/* PROTOTYPE_END */

#endif
/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
