#if !defined(YOG_MISC_H_INCLUDED)
#define YOG_MISC_H_INCLUDED

#include "yog/yog.h"

#define ADD_TO_LIST(list, elem)     do { \
    (elem)->prev = NULL; \
    (elem)->next = (list); \
    if ((list) != NULL) { \
        (list)->prev = (elem); \
    } \
    (list) = (elem); \
} while (0)

#define DELETE_FROM_LIST(list, elem)    do { \
    if ((elem)->prev != NULL) { \
        (elem)->prev->next = (elem)->next; \
    } \
    else { \
        (list) = (elem)->next; \
    } \
    if ((elem)->next != NULL) { \
        (elem)->next->prev = (elem)->prev; \
    } \
} while (0)

/* PROTOTYPE_START */

/**
 * DON'T EDIT THIS AREA. HERE IS GENERATED BY update_prototype.py.
 */
/* src/misc.c */
YOG_EXPORT void YogMisc_eval_source(YogEnv*, YogVal, const char*);
YOG_EXPORT YogHandle* YogMisc_format_method(YogEnv*, YogHandle*, YogHandle*);

/* PROTOTYPE_END */

#endif
/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
