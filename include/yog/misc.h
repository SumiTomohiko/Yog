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

#endif
/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
