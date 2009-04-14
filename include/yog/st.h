/* This is a public domain general purpose hash table package written by Peter Moore @ UCB. */

/* @(#) st.h 5.1 89/12/14 */

#if !defined(__YOG_ST_H__)
#define __YOG_ST_H__

#include "yog/yog.h"

struct YogHashType {
    int (*compare)(YogEnv*, YogVal, YogVal);
    int (*hash)(YogEnv*, YogVal);
};

typedef struct YogHashType YogHashType;

struct YogTableEntry {
    unsigned int hash;
    YogVal key;
    YogVal record;
    YogVal next;
};

typedef struct YogTableEntry YogTableEntry;

struct YogTableEntryArray {
    unsigned int size;
    YogVal items[0];
};

typedef struct YogTableEntryArray YogTableEntryArray;

struct YogTable {
    struct YogHashType* type;
    int num_bins;
    int num_entries;
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
void YogTable_add_direct(YogEnv*, YogVal, YogVal, YogVal);
void YogTable_cleanup_safe(YogEnv*, YogVal, YogVal*);
BOOL YogTable_delete(YogEnv*, YogVal, YogVal*, YogVal*);
BOOL YogTable_delete_safe(YogEnv*, YogVal, YogVal*, YogVal*, YogVal);
void YogTable_dump(YogEnv*, YogVal);
BOOL YogTable_foreach(YogEnv*, YogVal, int (*)(YogEnv*, YogVal, YogVal, YogVal*), YogVal*);
BOOL YogTable_insert(YogEnv*, YogVal, YogVal, YogVal);
BOOL YogTable_lookup(YogEnv*, YogVal, YogVal, YogVal*);
BOOL YogTable_lookup_str(YogEnv*, YogVal, const char*, YogVal*);
YogVal YogTable_new_string_table(YogEnv*);
YogVal YogTable_new_symbol_table(YogEnv*);
YogVal YogTable_new_val_table(YogEnv*);
int YogTable_size(YogEnv*, YogVal);

/* PROTOTYPE_END */

#endif
/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
