/* This is a public domain general purpose hash table package written by Peter Moore @ UCB. */

/* static        char        sccsid[] = "@(#) st.c 5.1 89/12/14 Crucible"; */

#include "config.h"
#if 0
#   include "defines.h"
#endif
#include <stdio.h>
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif
#include <string.h>
#include "yog/st.h"
#include "yog/yog.h"

#define ST_DEFAULT_MAX_DENSITY 5
#define ST_DEFAULT_INIT_TABLE_SIZE 11

    /*
     * DEFAULT_MAX_DENSITY is the default for the largest we allow the
     * average number of items per bin before increasing the number of
     * bins
     *
     * DEFAULT_INIT_TABLE_SIZE is the default for the number of bins
     * allocated initially
     *
     */

#define TABLE_ENTRY_TOP(table, i)   (table)->bins->items[(i)]

static void 
gc_bins_children(YogEnv* env, void* ptr, DoGc do_gc) 
{
    YogTableEntryArray* array = ptr;
    unsigned int size = array->size;
    unsigned int i = 0;
    for (i = 0; i < size; i++) {
        array->items[i] = do_gc(env, array->items[i]);
    }
}

static YogTableEntryArray*
alloc_bins(YogEnv* env, int size) 
{
    YogTableEntryArray* array = ALLOC_OBJ_ITEM(env, gc_bins_children, YogTableEntryArray, size, YogTableEntry*);
    bzero(array, sizeof(YogTableEntryArray) + size * sizeof(YogTableEntry*));

    array->size = size;

    return array;
}

/*
 * MINSIZE is the minimum size of a dictionary.
 */

#define MINSIZE 8

/*
Table of prime numbers 2^n+a, 2<=n<=30.
*/
static long primes[] = {
        8 + 3,
        16 + 3,
        32 + 5,
        64 + 3,
        128 + 3,
        256 + 27,
        512 + 9,
        1024 + 9,
        2048 + 5,
        4096 + 3,
        8192 + 27,
        16384 + 43,
        32768 + 3,
        65536 + 45,
        131072 + 29,
        262144 + 3,
        524288 + 21,
        1048576 + 7,
        2097152 + 17,
        4194304 + 15,
        8388608 + 9,
        16777216 + 43,
        33554432 + 35,
        67108864 + 15,
        134217728 + 29,
        268435456 + 3,
        536870912 + 11,
        1073741824 + 85,
        0
};

static int
new_size(int size)
{
    int i;

#if 0
    for (i=3; i<31; i++) {
        if ((1<<i) > size) return 1<<i;
    }
    return -1;
#else
    int newsize;

    for (i = 0, newsize = MINSIZE;
         i < sizeof(primes)/sizeof(primes[0]);
         i++, newsize <<= 1)
    {
        if (newsize > size) return primes[i];
    }
    /* Ran out of polynomials */
    return -1;                        /* should raise exception */
#endif
}

static void
rehash(YogEnv* env, YogTable* table)
{
    int old_num_bins = table->num_bins;
    int new_num_bins = new_size(old_num_bins + 1);
    YogTableEntryArray* new_bins = alloc_bins(env, new_num_bins);

    int i = 0;
    for(i = 0; i < old_num_bins; i++) {
        YogTableEntry* ptr = TABLE_ENTRY_TOP(table, i);
        while (ptr != NULL) {
            YogTableEntry* next = ptr->next;
            unsigned int hash_val = ptr->hash % new_num_bins;
            ptr->next = new_bins->items[hash_val];
            new_bins->items[hash_val] = ptr;
            ptr = next;
        }
    }

    table->num_bins = new_num_bins;
    table->bins = new_bins;
}

#define EQUAL(env, table, x, y) ((*table->type->compare)((env), (x), (y)) == 0)

#define do_hash(env, table, key) (unsigned int)(*(table)->type->hash)((env), (key))
#define do_hash_bin(env, table, key) (do_hash(env, table, key) % (table)->num_bins)

#ifdef HASH_LOG
static int collision = 0;
static int init_st = 0;

static void
stat_col()
{
    FILE *f = fopen("/tmp/col", "w");
    fprintf(f, "collision: %d\n", collision);
    fclose(f);
}
#endif

static void 
gc_table_children(YogEnv* env, void* ptr, DoGc do_gc) 
{
    YogTable* tbl = ptr;
    tbl->bins = do_gc(env, tbl->bins);
}

static YogTable*
alloc_table(YogEnv* env) 
{
    YogTable* tbl = ALLOC_OBJ(env, gc_table_children, YogTable);
    tbl->type = NULL;
    tbl->num_bins = 0;
    tbl->num_entries = 0;
    tbl->bins = NULL;

    return tbl;
}

static YogTable* 
st_init_table_with_size(YogEnv* env, YogHashType* type, int size)
{
    YogTable* tbl = NULL;

#ifdef HASH_LOG
    if (init_st == 0) {
        init_st = 1;
        atexit(stat_col);
    }
#endif

    size = new_size(size);        /* round up to prime number */

    tbl = alloc_table(env);
    tbl->type = type;
    tbl->num_entries = 0;
    tbl->num_bins = size;
    tbl->bins = alloc_bins(env, size);

    return tbl;
}

static YogTable* 
st_init_table(YogEnv* env, YogHashType* type)
{
    return st_init_table_with_size(env, type, 0);
}

#define PTR_NOT_EQUAL(env, table, ptr, hash_val, key) \
((ptr) != NULL && (ptr->hash != (hash_val) || !EQUAL((env), (table), (key), (ptr)->key)))

#ifdef HASH_LOG
#define COLLISION collision++
#else
#define COLLISION
#endif

#define FIND_ENTRY(env, table, ptr, hash_val, bin_pos) do {\
    bin_pos = hash_val % (table)->num_bins;\
    ptr = TABLE_ENTRY_TOP(table, bin_pos);\
    if (PTR_NOT_EQUAL(env, table, ptr, hash_val, key)) {\
        COLLISION;\
        while (PTR_NOT_EQUAL(env, table, ptr->next, hash_val, key)) {\
            ptr = ptr->next;\
        }\
        ptr = ptr->next;\
    }\
} while (0)

BOOL
YogTable_lookup(YogEnv* env, YogTable* table, YogVal key, YogVal* value) 
{
    unsigned int hash_val = do_hash(env, table, key);

    unsigned int bin_pos = 0;
    YogTableEntry* ptr = NULL;
    FIND_ENTRY(env, table, ptr, hash_val, bin_pos);

    if (ptr == NULL) {
        return FALSE;
    }
    else {
        if (value != NULL) {
            *value = ptr->record;
        }
        return TRUE;
    }
}

static void 
gc_table_entry_children(YogEnv* env, void* ptr, DoGc do_gc) 
{
    YogTableEntry* entry = ptr;
    entry->next = do_gc(env, entry->next);
}

static YogTableEntry* 
alloc_entry(YogEnv* env)
{
    YogTableEntry* entry = ALLOC_OBJ(env, gc_table_entry_children, YogTableEntry);
    entry->hash = 0;
    entry->key = YUNDEF;
    entry->record = YUNDEF;
    entry->next = NULL;

    return entry;
}

#define ADD_DIRECT(env, table, key, value, hash_val, bin_pos)\
do {\
    if (ST_DEFAULT_MAX_DENSITY < table->num_entries / (table->num_bins)) {\
        rehash(env, table);\
        bin_pos = hash_val % table->num_bins;\
    }\
    \
    YogTableEntry* entry = alloc_entry(env);\
    \
    entry->hash = hash_val;\
    entry->key = key;\
    entry->record = value;\
    entry->next = TABLE_ENTRY_TOP(table, bin_pos);\
    TABLE_ENTRY_TOP(table, bin_pos) = entry;\
    table->num_entries++;\
} while (0)

BOOL
YogTable_insert(YogEnv* env, YogTable* table, YogVal key, YogVal value) 
{
    unsigned int hash_val = do_hash(env, table, key);

    unsigned int bin_pos = 0;
    YogTableEntry* ptr = NULL;
    FIND_ENTRY(env, table, ptr, hash_val, bin_pos);

    if (ptr == NULL) {
        ADD_DIRECT(env, table, key, value, hash_val, bin_pos);
        return FALSE;
    }
    else {
        ptr->record = value;
        return TRUE;
    }
}

void
YogTable_add_direct(YogEnv* env, YogTable* table, YogVal key, YogVal value) 
{
    unsigned int hash_val = do_hash(env, table, key);
    unsigned int bin_pos = hash_val % table->num_bins;
    ADD_DIRECT(env, table, key, value, hash_val, bin_pos);
}

BOOL
YogTable_delete(YogEnv* env, YogTable* table, YogVal* key, YogVal* value) 
{
    unsigned int hash_val = do_hash_bin(env, table, *key);
    YogTableEntry* ptr = TABLE_ENTRY_TOP(table, hash_val);

    if (ptr == NULL) {
        if (value != NULL) {
            *value = YNIL;
        }
        return FALSE;
    }

    if (EQUAL(env, table, *key, ptr->key)) {
        TABLE_ENTRY_TOP(table, hash_val) = ptr->next;
        table->num_entries--;
        if (value != NULL) {
            *value = ptr->record;
        }
        *key = ptr->key;
        return TRUE;
    }

    for(; ptr->next != NULL; ptr = ptr->next) {
        if (EQUAL(env, table, ptr->next->key, *key)) {
            YogTableEntry* tmp = ptr->next;
            ptr->next = ptr->next->next;
            table->num_entries--;
            if (value != NULL) {
                *value = tmp->record;
            }
            *key = tmp->key;
            return TRUE;
        }
    }

    return FALSE;
}

BOOL
YogTable_delete_safe(YogEnv* env, YogTable* table, YogVal* key, YogVal* value, YogVal never)
{
    unsigned int hash_val = do_hash_bin(env, table, *key);
    YogTableEntry* ptr = TABLE_ENTRY_TOP(table, hash_val);

    if (ptr == NULL) {
        if (value != NULL) {
            *value = YNIL;
        }
        return FALSE;
    }

    for(; ptr != NULL; ptr = ptr->next) {
        if (!YogVal_equals_exact(env, ptr->key, never) && EQUAL(env, table, ptr->key, *key)) {
            table->num_entries--;
            *key = ptr->key;
            if (value != NULL) {
                *value = ptr->record;
            }
            ptr->key = ptr->record = never;
            return TRUE;
        }
    }

    return FALSE;
}

static int
delete_never(YogEnv* env, YogVal key, YogVal value, YogVal* never)
{
    if (YogVal_equals_exact(env, value, *never)) {
        return ST_DELETE;
    }
    else {
        return ST_CONTINUE;
    }
}

BOOL
YogTable_foreach(YogEnv* env, YogTable* table, int (*func)(YogEnv*, YogVal, YogVal, YogVal*), YogVal* arg)
{
    int i = 0;
    for (i = 0; i < table->num_bins; i++) {
        YogTableEntry* last = NULL;
        YogTableEntry* ptr = NULL;
        for (ptr = TABLE_ENTRY_TOP(table, i); ptr != NULL;) {
            enum st_retval retval = (*func)(env, ptr->key, ptr->record, arg);
            YogTableEntry* tmp = NULL;
            switch (retval) {
                case ST_CHECK:        /* check if hash is modified during iteration */
                    tmp = NULL;
                    if (i < table->num_bins) {
                        for (tmp = TABLE_ENTRY_TOP(table, i); tmp != NULL; tmp = tmp->next) {
                            if (tmp == ptr) {
                                break;
                            }
                        }
                    }
                    if (tmp == NULL) {
                        /* call func with error notice */
                        return FALSE;
                    }
                    /* fall through */
                case ST_CONTINUE:
                    last = ptr;
                    ptr = ptr->next;
                    break;
                case ST_STOP:
                    return TRUE;
                    break;
                case ST_DELETE:
                    tmp = ptr;
                    if (last == NULL) {
                        TABLE_ENTRY_TOP(table, i) = ptr->next;
                    }
                    else {
                        last->next = ptr->next;
                    }
                    ptr = ptr->next;
                    table->num_entries--;
                    break;
            }
        }
    }

    return TRUE;
}

void
YogTable_cleanup_safe(YogEnv* env, YogTable* table, YogVal* never)
{
    int num_entries = table->num_entries;

    YogTable_foreach(env, table, delete_never, never);
    table->num_entries = num_entries;
}

static int 
compare_symbol(YogEnv* env, YogVal a, YogVal b) 
{
    return VAL2ID(a) - VAL2ID(b);
}

static int 
hash_symbol(YogEnv* env, YogVal key) 
{
    return VAL2ID(key);
}

static YogHashType type_symbol = {
    compare_symbol, 
    hash_symbol
};

YogTable* 
YogTable_new_symbol_table(YogEnv* env)
{
    return st_init_table(env, &type_symbol);
}

static int 
compare_string(YogEnv* env, YogVal a, YogVal b) 
{
#define GET_STR(val)    (((YogCharArray*)VAL2PTR(a))->items)
    return strcmp(GET_STR(a), GET_STR(b));
#undef GET_STR
}

static int
strhash(const char* string)
{
    register int c;

#ifdef HASH_ELFHASH
    register unsigned int h = 0, g;

    while ((c = *string++) != '\0') {
        h = ( h << 4 ) + c;
        if ( g = h & 0xF0000000 )
            h ^= g >> 24;
        h &= ~g;
    }
    return h;
#elif defined(HASH_PERL)
    register int val = 0;

    while ((c = *string++) != '\0') {
        val += c;
        val += (val << 10);
        val ^= (val >> 6);
    }
    val += (val << 3);
    val ^= (val >> 11);

    return val + (val << 15);
#else
    register int val = 0;

    while ((c = *string++) != '\0') {
        val = val*997 + c;
    }

    return val + (val>>5);
#endif
}

static int 
hash_string(YogEnv* env, YogVal key) 
{
    YogCharArray* array = VAL2PTR(key);
    return strhash(array->items);
}

static YogHashType type_string = {
    compare_string, 
    hash_string, 
};

YogTable* 
YogTable_new_string_table(YogEnv* env) 
{
    return st_init_table(env, &type_string);
}

BOOL
YogTable_lookup_str(YogEnv* env, YogTable* table, const char* key, YogVal* value) 
{
    YOG_ASSERT(env, table->type == &type_string, "Table type must be type_string.");

    unsigned int hash_val = strhash(key);
    unsigned int bin_pos = hash_val % table->num_bins;
    YogTableEntry* entry = TABLE_ENTRY_TOP(table, bin_pos);

#define NOT_EQUAL_ENTRY ((entry != NULL) && ((entry->hash != hash_val) || (strcmp(((YogCharArray*)VAL2PTR(entry->key))->items, key) != 0)))
    if (NOT_EQUAL_ENTRY) {
        COLLISION;
        do {
            entry = entry->next;
        } while (NOT_EQUAL_ENTRY);
    }
#undef EQUAL_ENTRY

    if (entry != NULL) {
        if (value != NULL) {
            *value = entry->record;
        }
        return TRUE;
    }
    else {
        return FALSE;
    }
}

static int 
compare_val(YogEnv* env, YogVal a, YogVal b) 
{
    return YogVal_equals_exact(env, a, b) ? 0 : 1;
}

static int 
hash_val(YogEnv* env, YogVal val) 
{
    return YogVal_hash(env, val);
}

static YogHashType type_val = {
    compare_val, 
    hash_val, 
};

YogTable* 
YogTable_new_val_table(YogEnv* env) 
{
    return st_init_table(env, &type_val);
}

int 
YogTable_size(YogEnv* env, YogTable* table) 
{
    return table->num_entries;
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
