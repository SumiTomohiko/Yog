#if !defined(YOG_ARG_H_INCLUDED)
#define YOG_ARG_H_INCLUDED

#include "yog/yog.h"

struct YogArgInfo {
    uint_t argc;
    uint_t varargc;
    uint_t kwargc;
    uint_t blockargc;

    uint_t required_argc;
    YogVal argnames;
};

typedef struct YogArgInfo YogArgInfo;

#define ARG_INFO(v)     PTR_AS(YogArgInfo, (v))

/* PROTOTYPE_START */

/**
 * DON'T EDIT THIS AREA. HERE IS GENERATED BY update_prototype.py.
 */
/* src/arg.c */
YOG_EXPORT YogVal YogArgInfo_new(YogEnv*);

/* PROTOTYPE_END */

#endif
/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
