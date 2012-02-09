/* This is a public domain general purpose hash table package written by Peter Moore @ UCB. */

/* @(#) st.h 5.1 89/12/14 */

#if !defined(YOG_ST_H_INCLUDED)
#define YOG_ST_H_INCLUDED

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
YogVal YogTableIterator_current_key(YogEnv*, YogVal);
YogVal YogTableIterator_current_value(YogEnv*, YogVal);
BOOL YogTableIterator_next(YogEnv*, YogVal);
void YogTable_add_direct(YogEnv*, YogVal, YogVal, YogVal);
YogVal YogTable_create_string_table(YogEnv*);
YogVal YogTable_create_symbol_table(YogEnv*);
YogVal YogTable_create_value_table(YogEnv*);
BOOL YogTable_delete(YogEnv*, YogVal, YogVal*, YogVal*);
void YogTable_dump(YogEnv*, YogVal);
BOOL YogTable_foreach(YogEnv*, YogVal, int_t (*)(YogEnv*, YogVal, YogVal, YogVal*), YogVal*);
YogVal YogTable_get_iterator(YogEnv*, YogVal);
BOOL YogTable_insert(YogEnv*, YogVal, YogVal, YogVal);
BOOL YogTable_lookup(YogEnv*, YogVal, YogVal, YogVal*);
BOOL YogTable_lookup_sym(YogEnv*, YogVal, YogVal, YogVal*);
void YogTable_raw_dump(YogEnv*, YogVal);
int_t YogTable_size(YogEnv*, YogVal);

/* PROTOTYPE_END */

#endif
/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
