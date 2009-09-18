#if !defined(__YOG_MISC_H__)
#define __YOG_MISC_H__

#if 0
#define ADD_TO_LIST(list, elem)     do { \
    (elem)->prev = NULL; \
    (elem)->next = (list); \
    if ((list) != NULL) { \
        (list)->prev = elem; \
    } \
    (list) = (elem); \
} while (0)
#endif

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

#if !defined(HAVE_STRLCPY)
#   define strlcpy(dst, src, size)  strcpy(dst, src)
#endif
#if !defined(HAVE_STRLCAT)
#   define strlcat(dst, src, size)  strcat(dst, src)
#endif

/* PROTOTYPE_START */

/**
 * DON'T EDIT THIS AREA. HERE IS GENERATED BY update_prototype.py.
 */

/* src/misc.c */
void YogMisc_eval_source(YogEnv*, YogVal, const char*);

/* PROTOTYPE_END */

#endif
/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
