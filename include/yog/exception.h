#ifndef __YOG_EXCEPTION_H__
#define __YOG_EXCEPTION_H__

#include "yog/yog.h"

struct YogStackTraceEntry {
    struct YogVal lower;
    unsigned int lineno;
    YogVal filename;
    ID klass_name;
    ID func_name;
};

typedef struct YogStackTraceEntry YogStackTraceEntry;

struct YogException {
    YOGBASICOBJ_HEAD;
    struct YogVal stack_trace;
    struct YogVal message;
};

typedef struct YogException YogException;

/* PROTOTYPE_START */

/**
 * DON'T EDIT THIS AREA. HERE IS GENERATED BY update_prototype.py.
 */

/* src/exception.c */
YogVal YogBugException_new(YogEnv*);
YogVal YogException_klass_new(YogEnv*);

/* src/stacktrace.c */
YogVal YogStackTraceEntry_new(YogEnv*);

/* PROTOTYPE_END */

#endif
/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
