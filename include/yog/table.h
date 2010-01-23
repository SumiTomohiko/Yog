/* This is a public domain general purpose hash table package written by Peter Moore @ UCB. */

/* @(#) st.h 5.1 89/12/14 */

#if !defined(__YOG_ST_H__)
#define __YOG_ST_H__

#include "yog/yog.h"

struct YogHashType {
    BOOL (*compare)(YogEnv*, YogVal, YogVal);
    int_t (*hash)(YogEnv*, YogVal);
};

typedef struct YogHashType YogHashType;

struct YogTableEntry {
    uint_t hash;
    YogVal key;
    YogVal record;
    YogVal next;
};

typedef struct YogTableEntry YogTableEntry;

struct YogTableEntryArray {
    uint_t size;
    YogVal items[0];
};

typedef struct YogTableEntryArray YogTableEntryArray;

struct YogTable {
    struct YogHashType* type;
    int_t num_bins;
    int_t num_entries;
    YogVal bins;
};

typedef struct YogTable YogTable;

enum st_retval {
    ST_CONTINUE,
    ST_STOP,
    ST_DELETE,
    ST_CHECK
};

/* PROTOTYPE_START */

/**
 * DON'T EDIT THIS AREA. HERE IS GENERATED BY update_prototype.py.
 */
/* src/table.c */
YOG_EXPORT YogVal YogTableIterator_current_key(YogEnv*, YogVal);
YOG_EXPORT YogVal YogTableIterator_current_value(YogEnv*, YogVal);
YOG_EXPORT BOOL YogTableIterator_next(YogEnv*, YogVal);
YOG_EXPORT void YogTable_add_direct(YogEnv*, YogVal, YogVal, YogVal);
YOG_EXPORT BOOL YogTable_delete(YogEnv*, YogVal, YogVal*, YogVal*);
YOG_EXPORT void YogTable_dump(YogEnv*, YogVal);
YOG_EXPORT BOOL YogTable_foreach(YogEnv*, YogVal, int_t (*)(YogEnv*, YogVal, YogVal, YogVal*), YogVal*);
YOG_EXPORT YogVal YogTable_get_iterator(YogEnv*, YogVal);
YOG_EXPORT BOOL YogTable_insert(YogEnv*, YogVal, YogVal, YogVal);
YOG_EXPORT BOOL YogTable_lookup(YogEnv*, YogVal, YogVal, YogVal*);
YOG_EXPORT BOOL YogTable_lookup_str(YogEnv*, YogVal, const char*, YogVal*);
YOG_EXPORT YogVal YogTable_new_string_table(YogEnv*);
YOG_EXPORT YogVal YogTable_new_symbol_table(YogEnv*);
YOG_EXPORT YogVal YogTable_new_val_table(YogEnv*);
YOG_EXPORT void YogTable_raw_dump(YogEnv*, YogVal);
YOG_EXPORT int_t YogTable_size(YogEnv*, YogVal);

/* PROTOTYPE_END */

#endif
/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
