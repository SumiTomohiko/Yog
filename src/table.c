/* This is a public domain general purpose hash table package written by Peter Moore @ UCB. */

/* static        char        sccsid[] = "@(#) st.c 5.1 89/12/14 Crucible"; */

#include "yog/config.h"
#if 0
#   include "defines.h"
#endif
#include <ctype.h>
#include <stdio.h>
#if defined(YOG_HAVE_STDLIB_H)
#include <stdlib.h>
#endif
#include <string.h>
#include "yog/binary.h"
#include "yog/error.h"
#include "yog/eval.h"
#include "yog/gc.h"
#include "yog/string.h"
#include "yog/table.h"
#include "yog/thread.h"
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
#define TABLE_BINS(table) \
                    PTR_AS(YogTable, (table))->bins
#define TABLE_ENTRY_TOP(table, i) \
                    PTR_AS(YogTableEntryArray, TABLE_BINS((table)))->items[(i)]

static void
keep_bins_children(YogEnv* env, void* ptr, ObjectKeeper keeper, void* heap)
{
    YogTableEntryArray* array = PTR_AS(YogTableEntryArray, ptr);
    uint_t size = array->size;
    uint_t i;
    for (i = 0; i < size; i++) {
        YogGC_KEEP(env, array, items[i], keeper, heap);
    }
}

static YogVal
alloc_bins(YogEnv* env, int_t size)
{
    YogGC_check_multiply_overflow(env, size, sizeof(YogVal));
    YogVal array = ALLOC_OBJ_ITEM(env, keep_bins_children, NULL, YogTableEntryArray, size, YogVal);

    PTR_AS(YogTableEntryArray, array)->size = size;
    int_t i;
    for (i = 0; i < size; i++) {
        PTR_AS(YogTableEntryArray, array)->items[i] = YNIL;
    }

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

static int_t
new_size(int_t size)
{
    int_t i;

#if 0
    for (i=3; i<31; i++) {
        if ((1<<i) > size) return 1<<i;
    }
    return -1;
#else
    int_t newsize;

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
rehash(YogEnv* env, YogVal table)
{
    SAVE_ARG(env, table);
    YogVal new_bins = YUNDEF;
    YogVal ptr = YUNDEF;
    YogVal next = YUNDEF;
    PUSH_LOCALS3(env, new_bins, ptr, next);

    int_t old_num_bins = PTR_AS(YogTable, table)->num_bins;
    int_t new_num_bins = new_size(old_num_bins + 1);
    new_bins = alloc_bins(env, new_num_bins);

    int_t i;
    for(i = 0; i < old_num_bins; i++) {
        ptr = TABLE_ENTRY_TOP(table, i);
        while (IS_PTR(ptr)) {
            next = PTR_AS(YogTableEntry, ptr)->next;
            uint_t hash_val = PTR_AS(YogTableEntry, ptr)->hash % new_num_bins;
            YogGC_UPDATE_PTR(env, PTR_AS(YogTableEntry, ptr), next, PTR_AS(YogTableEntryArray, new_bins)->items[hash_val]);
            YogGC_UPDATE_PTR(env, PTR_AS(YogTableEntryArray, new_bins), items[hash_val], ptr);
            ptr = next;
        }
    }

    PTR_AS(YogTable, table)->num_bins = new_num_bins;
    YogGC_UPDATE_PTR(env, PTR_AS(YogTable, table), bins, new_bins);

    RETURN_VOID(env);
}

#define EQUAL(env, table, x, y) \
    PTR_AS(YogTable, table)->type->compare((env), (x), (y))

#define do_hash(env, table, key) (uint_t)(*PTR_AS(YogTable, (table))->type->hash)((env), (key))
#define do_hash_bin(env, table, key) (do_hash(env, table, key) % PTR_AS(YogTable, table)->num_bins)

#if defined(HASH_LOG)
static int_t collision = 0;
static int_t init_st = 0;

static void
stat_col()
{
    FILE *f = fopen("/tmp/col", "w");
    fprintf(f, "collision: %d\n", collision);
    fclose(f);
}
#endif

static void
YogTable_keep_children(YogEnv* env, void* ptr, ObjectKeeper keeper, void* heap)
{
    YogTable* tbl = PTR_AS(YogTable, ptr);
    YogGC_KEEP(env, tbl, bins, keeper, heap);
}

static YogVal
alloc_table(YogEnv* env)
{
    YogVal tbl = ALLOC_OBJ(env, YogTable_keep_children, NULL, YogTable);
    PTR_AS(YogTable, tbl)->type = NULL;
    PTR_AS(YogTable, tbl)->num_bins = 0;
    PTR_AS(YogTable, tbl)->num_entries = 0;
    PTR_AS(YogTable, tbl)->bins = YNIL;

    return tbl;
}

static YogVal
st_init_table_with_size(YogEnv* env, YogHashType* type, int_t size)
{
    SAVE_LOCALS(env);
    YogVal tbl = YUNDEF;
    YogVal bins = YUNDEF;
    PUSH_LOCALS2(env, tbl, bins);

#if defined(HASH_LOG)
    if (init_st == 0) {
        init_st = 1;
        atexit(stat_col);
    }
#endif

    size = new_size(size);        /* round up to prime number */

    tbl = alloc_table(env);
    PTR_AS(YogTable, tbl)->type = type;
    PTR_AS(YogTable, tbl)->num_entries = 0;
    PTR_AS(YogTable, tbl)->num_bins = size;
    bins = alloc_bins(env, size);
    YogGC_UPDATE_PTR(env, PTR_AS(YogTable, tbl), bins, bins);

    RETURN(env, tbl);
}

static YogVal
st_init_table(YogEnv* env, YogHashType* type)
{
    return st_init_table_with_size(env, type, 0);
}

static BOOL
ptr_not_equal(YogEnv* env, YogVal table, YogVal ptr, uint_t hash_val, YogVal key)
{
    if (!IS_PTR(ptr)) {
        return FALSE;
    }
    if (PTR_AS(YogTableEntry, ptr)->hash != hash_val) {
        return TRUE;
    }
    return !EQUAL(env, table, key, PTR_AS(YogTableEntry, ptr)->key);
}

#if defined(HASH_LOG)
#define COLLISION collision++
#else
#define COLLISION
#endif

inline static void
find_entry(YogEnv* env, YogVal table, YogVal* ptr, uint_t hash_val, uint_t* bin_pos, YogVal key)
{
    SAVE_ARGS2(env, table, key);

    *bin_pos = hash_val % PTR_AS(YogTable, table)->num_bins;
    *ptr = TABLE_ENTRY_TOP(table, *bin_pos);
    if (ptr_not_equal(env, table, *ptr, hash_val, key)) {
        COLLISION;
        while (ptr_not_equal(env, table, PTR_AS(YogTableEntry, (*ptr))->next, hash_val, key)) {
            *ptr = PTR_AS(YogTableEntry, (*ptr))->next;
        }
        *ptr = PTR_AS(YogTableEntry, (*ptr))->next;
    }

    RETURN_VOID(env);
}

BOOL
YogTable_lookup_sym(YogEnv* env, YogVal table, YogVal key, YogVal* value)
{
    /* YogTable_lookup without GC guard */
    uint_t hash_val = do_hash(env, table, key);

    YogVal ptr = YUNDEF;
    uint_t bin_pos = 0;
    find_entry(env, table, &ptr, hash_val, &bin_pos, key);

    if (!IS_PTR(ptr)) {
        return FALSE;
    }
    if (value != NULL) {
        *value = PTR_AS(YogTableEntry, ptr)->record;
    }
    return TRUE;
}

BOOL
YogTable_lookup(YogEnv* env, YogVal table, YogVal key, YogVal* value)
{
    SAVE_ARGS2(env, table, key);
    YogVal ptr = YNIL;
    PUSH_LOCAL(env, ptr);

    uint_t hash_val = do_hash(env, table, key);

    uint_t bin_pos = 0;
    find_entry(env, table, &ptr, hash_val, &bin_pos, key);

    if (!IS_PTR(ptr)) {
        RETURN(env, FALSE);
    }
    if (value != NULL) {
        *value = PTR_AS(YogTableEntry, ptr)->record;
    }
    RETURN(env, TRUE);
}

static void
YogTableEntry_keep_children(YogEnv* env, void* ptr, ObjectKeeper keeper, void* heap)
{
    YogTableEntry* entry = PTR_AS(YogTableEntry, ptr);
#define KEEP(member)    YogGC_KEEP(env, entry, member, keeper, heap)
    KEEP(key);
    KEEP(record);
    KEEP(next);
#undef KEEP
}

static YogVal
alloc_entry(YogEnv* env)
{
    YogVal entry = ALLOC_OBJ(env, YogTableEntry_keep_children, NULL, YogTableEntry);
    PTR_AS(YogTableEntry, entry)->hash = 0;
    PTR_AS(YogTableEntry, entry)->key = YUNDEF;
    PTR_AS(YogTableEntry, entry)->record = YUNDEF;
    PTR_AS(YogTableEntry, entry)->next = YUNDEF;

    return entry;
}

static void
add_direct(YogEnv* env, YogVal table, YogVal key, YogVal value, uint_t hash_val, uint_t bin_pos)
{
    SAVE_ARGS3(env, table, key, value);
    YogVal entry = YUNDEF;
    PUSH_LOCAL(env, entry);

    if (ST_DEFAULT_MAX_DENSITY < PTR_AS(YogTable, table)->num_entries / (PTR_AS(YogTable, table)->num_bins)) {
        rehash(env, table);
        bin_pos = hash_val % PTR_AS(YogTable, table)->num_bins;
    }

    entry = alloc_entry(env);

    PTR_AS(YogTableEntry, entry)->hash = hash_val;
    YogGC_UPDATE_PTR(env, PTR_AS(YogTableEntry, entry), key, key);
    YogGC_UPDATE_PTR(env, PTR_AS(YogTableEntry, entry), record, value);
    YogGC_UPDATE_PTR(env, PTR_AS(YogTableEntry, entry), next, TABLE_ENTRY_TOP(table, bin_pos));

    YogGC_UPDATE_PTR(env, PTR_AS(YogTableEntryArray, TABLE_BINS(table)), items[bin_pos], entry);
    PTR_AS(YogTable, table)->num_entries++;

    RETURN_VOID(env);
}

BOOL
YogTable_insert(YogEnv* env, YogVal table, YogVal key, YogVal value)
{
    SAVE_ARGS3(env, table, key, value);
    YogVal ptr = YNIL;
    PUSH_LOCAL(env, ptr);

    uint_t hash_val = do_hash(env, table, key);

    uint_t bin_pos = 0;
    find_entry(env, table, &ptr, hash_val, &bin_pos, key);

    if (!IS_PTR(ptr)) {
        add_direct(env, table, key, value, hash_val, bin_pos);
        RETURN(env, FALSE);
    }
    else {
        YogGC_UPDATE_PTR(env, PTR_AS(YogTableEntry, ptr), record, value);
        RETURN(env, TRUE);
    }
}

void
YogTable_add_direct(YogEnv* env, YogVal table, YogVal key, YogVal value)
{
    SAVE_ARGS3(env, table, key, value);

    uint_t hash_val = do_hash(env, table, key);
    uint_t bin_pos = hash_val % PTR_AS(YogTable, table)->num_bins;
    add_direct(env, table, key, value, hash_val, bin_pos);

    RETURN_VOID(env);
}

BOOL
YogTable_delete(YogEnv* env, YogVal table, YogVal* key, YogVal* value)
{
    SAVE_ARG(env, table);
    YogVal ptr = YUNDEF;
    YogVal next = YUNDEF;
    YogVal tmp = YUNDEF;
    PUSH_LOCALS3(env, ptr, next, tmp);

    uint_t hash_val = do_hash_bin(env, table, *key);
    ptr = TABLE_ENTRY_TOP(table, hash_val);

    if (!IS_PTR(ptr)) {
        if (value != NULL) {
            *value = YNIL;
        }
        RETURN(env, FALSE);
    }

    if (EQUAL(env, table, *key, PTR_AS(YogTableEntry, ptr)->key)) {
        YogGC_UPDATE_PTR(env, PTR_AS(YogTableEntryArray, TABLE_BINS(table)), items[hash_val], PTR_AS(YogTableEntry, ptr)->next);
        PTR_AS(YogTable, table)->num_entries--;
        if (value != NULL) {
            *value = PTR_AS(YogTableEntry, ptr)->record;
        }
        *key = PTR_AS(YogTableEntry, ptr)->key;
        RETURN(env, TRUE);
    }

    for(; IS_PTR(PTR_AS(YogTableEntry, ptr)->next); ptr = PTR_AS(YogTableEntry, ptr)->next) {
        next = PTR_AS(YogTableEntry, ptr)->next;
        if (EQUAL(env, table, PTR_AS(YogTableEntry, next)->key, *key)) {
            tmp = next;
            YogGC_UPDATE_PTR(env, PTR_AS(YogTableEntry, ptr), next, PTR_AS(YogTableEntry, next)->next);
            PTR_AS(YogTable, table)->num_entries--;
            if (value != NULL) {
                *value = PTR_AS(YogTableEntry, tmp)->record;
            }
            *key = PTR_AS(YogTableEntry, tmp)->key;
            RETURN(env, TRUE);
        }
    }

    RETURN(env, FALSE);
}

BOOL
YogTable_foreach(YogEnv* env, YogVal table, int_t (*func)(YogEnv*, YogVal, YogVal, YogVal*), YogVal* arg)
{
    SAVE_ARG(env, table);
    YogVal last = YUNDEF;
    YogVal ptr = YUNDEF;
    YogVal tmp = YUNDEF;
    PUSH_LOCALS3(env, last, ptr, tmp);

    int_t i;
    for (i = 0; i < PTR_AS(YogTable, table)->num_bins; i++) {
        for (ptr = TABLE_ENTRY_TOP(table, i); IS_PTR(ptr);) {
            enum st_retval retval = (enum st_retval)func(env, PTR_AS(YogTableEntry, ptr)->key, PTR_AS(YogTableEntry, ptr)->record, arg);
            switch (retval) {
                case ST_CHECK:        /* check if hash is modified during iteration */
                    tmp = YNIL;
                    if (i < PTR_AS(YogTable, table)->num_bins) {
                        for (tmp = TABLE_ENTRY_TOP(table, i); IS_PTR(tmp); tmp = PTR_AS(YogTableEntry, tmp)->next) {
                            if (VAL2PTR(tmp) == VAL2PTR(ptr)) {
                                break;
                            }
                        }
                    }
                    if (!IS_PTR(tmp)) {
                        /* call func with error notice */
                        RETURN(env, FALSE);
                    }
                    /* fall through */
                case ST_CONTINUE:
                    last = ptr;
                    ptr = PTR_AS(YogTableEntry, ptr)->next;
                    break;
                case ST_STOP:
                    RETURN(env, TRUE);
                    break;
                case ST_DELETE:
                    tmp = ptr;
                    if (!IS_PTR(last)) {
                        YogGC_UPDATE_PTR(env, PTR_AS(YogTableEntryArray, TABLE_BINS(table)), items[i], PTR_AS(YogTableEntry, ptr)->next);
                    }
                    else {
                        YogGC_UPDATE_PTR(env, PTR_AS(YogTableEntry, last), next, PTR_AS(YogTableEntry, ptr)->next);
                    }
                    ptr = PTR_AS(YogTableEntry, ptr)->next;
                    PTR_AS(YogTable, table)->num_entries--;
                    break;
            }
        }
    }

    RETURN(env, TRUE);
}

static BOOL
compare_symbol(YogEnv* env, YogVal a, YogVal b)
{
    if (a == b) {
        return TRUE;
    }
    else {
        return FALSE;
    }
}

static int_t
hash_symbol(YogEnv* env, YogVal key)
{
    return VAL2ID(key);
}

static YogHashType type_symbol = {
    compare_symbol,
    hash_symbol
};

YogVal
YogTable_create_symbol_table(YogEnv* env)
{
    return st_init_table(env, &type_symbol);
}

static BOOL
compare_string(YogEnv* env, YogVal a, YogVal b)
{
    if (STRING_SIZE(a) != STRING_SIZE(b)) {
        return FALSE;
    }
    uint_t n = sizeof(YogChar) * STRING_SIZE(a);
    return memcmp(STRING_CHARS(a), STRING_CHARS(b), n) == 0 ? TRUE : FALSE;
}

static int_t
hash_string(YogEnv* env, YogVal key)
{
    return YogString_hash(env, key);
}

static YogHashType type_string = {
    compare_string,
    hash_string,
};

YogVal
YogTable_create_string_table(YogEnv* env)
{
    return st_init_table(env, &type_string);
}

static BOOL
compare_val(YogEnv* env, YogVal a, YogVal b)
{
    SAVE_ARGS2(env, a, b);
    YogVal val = YUNDEF;
    PUSH_LOCAL(env, val);

    YogJmpBuf jmpbuf;
    int_t status = setjmp(jmpbuf.buf);
    if (status != 0) {
        RETURN(env, FALSE);
    }
    INIT_JMPBUF(env, jmpbuf);
    PUSH_JMPBUF(env->thread, jmpbuf);
    val = YogEval_call_method1(env, a, "==", b);
    POP_JMPBUF(env);

    RETURN(env, YOG_TEST(val) ? TRUE : FALSE);
}

static int_t
hash_val(YogEnv* env, YogVal val)
{
    SAVE_ARG(env, val);
    YogVal hash = YUNDEF;
    PUSH_LOCAL(env, hash);

    hash = YogEval_call_method0(env, val, "hash");
    int_t h = YogVal_to_signed_type(env, hash, "hash");

    RETURN(env, h);
}

static YogHashType type_val = {
    compare_val,
    hash_val,
};

YogVal
YogTable_create_value_table(YogEnv* env)
{
    return st_init_table(env, &type_val);
}

int_t
YogTable_size(YogEnv* env, YogVal table)
{
    return PTR_AS(YogTable, table)->num_entries;
}

/**
 * TODO: commonize with Code#dump.
 */
static void
print_val(YogEnv* env, YogVal val)
{
    printf("%s:%d val=0x%08zx\n", __FILE__, __LINE__, val);
    if (IS_UNDEF(val)) {
        printf("undef");
    }
    else if (IS_PTR(val)) {
        printf("%p", VAL2PTR(val));
    }
    else if (IS_FIXNUM(val)) {
        printf("%zd", VAL2INT(val));
    }
    else if (IS_BOOL(val)) {
        if (VAL2BOOL(val)) {
            printf("true");
        }
        else {
            printf("false");
        }
    }
    else if (IS_NIL(val)) {
        printf("nil");
    }
    else if (IS_SYMBOL(val)) {
        printf(" 0x%08zx", VAL2ID(val));
#if 0
        printf(" :%s", YogVM_id2name(env, env->vm, VAL2ID(val)));
#endif
    }
    else {
        YOG_ASSERT(env, FALSE, "Unknown value type.");
    }
}

static int_t
dump_callback(YogEnv* env, YogVal key, YogVal value, YogVal* arg)
{
    printf("  ");
    print_val(env, key);
    printf(" => ");
    print_val(env, value);
    printf(", \n");

    return ST_CONTINUE;
}

static int_t
dump_string_callback(YogEnv* env, YogVal key, YogVal value, YogVal* arg)
{
    if (IS_PTR(key)) {
        YogHandle* h = YogHandle_REGISTER(env, key);
        YogVal s = YogString_to_bin_in_default_encoding(env, h);
        printf("  \"%s\" => ", BINARY_CSTR(s));
    }
    else {
        printf("  undef => ");
    }
    print_val(env, value);
    printf(", \n");

    return ST_CONTINUE;
}

void
YogTable_dump(YogEnv* env, YogVal table)
{
    if (!IS_PTR(table)) {
        return;
    }

    printf("{");
    if (PTR_AS(YogTable, table)->type == &type_val) {
        YogTable_foreach(env, table, dump_callback, NULL);
    }
    else if (PTR_AS(YogTable, table)->type == &type_string) {
        YogTable_foreach(env, table, dump_string_callback, NULL);
    }
    printf("}");
}

static int_t
raw_dump_callback(YogEnv* env, YogVal key, YogVal value, YogVal* arg)
{
    printf("  0x%08zx => 0x%08zx, \n", key, value);
    return ST_CONTINUE;
}

void
YogTable_raw_dump(YogEnv* env, YogVal table)
{
    if (!IS_PTR(table)) {
        return;
    }

    printf("{");
    YogTable_foreach(env, table, raw_dump_callback, NULL);
    printf("}");
}

struct TableIterator {
    YogVal tbl;
    uint_t bins_index;
    YogVal entry;
};

typedef struct TableIterator TableIterator;

static void
TableIterator_keep_children(YogEnv* env, void* ptr, ObjectKeeper keeper, void* heap)
{
    TableIterator* iter = PTR_AS(TableIterator, ptr);
#define KEEP(member)    YogGC_KEEP(env, iter, member, keeper, heap)
    KEEP(tbl);
    KEEP(entry);
#undef KEEP
}

YogVal
YogTableIterator_current_value(YogEnv* env, YogVal self)
{
    SAVE_ARG(env, self);
    YogVal value = YUNDEF;
    YogVal entry = YUNDEF;
    PUSH_LOCALS2(env, value, entry);

    entry = PTR_AS(TableIterator, self)->entry;
    value = PTR_AS(YogTableEntry, entry)->record;

    RETURN(env, value);
}

YogVal
YogTableIterator_current_key(YogEnv* env, YogVal self)
{
    SAVE_ARG(env, self);
    YogVal key = YUNDEF;
    YogVal entry = YUNDEF;
    PUSH_LOCALS2(env, key, entry);

    entry = PTR_AS(TableIterator, self)->entry;
    key = PTR_AS(YogTableEntry, entry)->key;

    RETURN(env, key);
}

BOOL
YogTableIterator_next(YogEnv* env, YogVal self)
{
    SAVE_ARG(env, self);
    YogVal current_entry = YUNDEF;
    YogVal entry = YUNDEF;
    YogVal next = YUNDEF;
    YogVal bins = YUNDEF;
    PUSH_LOCALS4(env, current_entry, entry, next, bins);

    current_entry = PTR_AS(TableIterator, self)->entry;
    if (IS_PTR(current_entry)) {
        next = PTR_AS(YogTableEntry, current_entry)->next;
        if (IS_PTR(next)) {
            YogGC_UPDATE_PTR(env, PTR_AS(TableIterator, self), entry, next);
            RETURN(env, TRUE);
        }
    }

    uint_t i = PTR_AS(TableIterator, self)->bins_index;
    YogVal tbl = PTR_AS(TableIterator, self)->tbl;
    uint_t num_bins = PTR_AS(YogTable, tbl)->num_bins;
    while (i < num_bins) {
        bins = PTR_AS(YogTable, tbl)->bins;
        entry = PTR_AS(YogTableEntryArray, bins)->items[i];
        if (IS_PTR(entry)) {
            break;
        }

        i++;
    }
    if (i < num_bins) {
        PTR_AS(TableIterator, self)->bins_index = i + 1;
        YogGC_UPDATE_PTR(env, PTR_AS(TableIterator, self), entry, entry);
        RETURN(env, TRUE);
    }

    PTR_AS(TableIterator, self)->bins_index = i;

    RETURN(env, FALSE);
}

YogVal
YogTable_get_iterator(YogEnv* env, YogVal self)
{
    SAVE_ARG(env, self);
    YogVal iter = YUNDEF;
    PUSH_LOCAL(env, iter);

    iter = ALLOC_OBJ(env, TableIterator_keep_children, NULL, TableIterator);
    YogGC_UPDATE_PTR(env, PTR_AS(TableIterator, iter), tbl, self);
    PTR_AS(TableIterator, iter)->bins_index = 0;
    PTR_AS(TableIterator, iter)->entry = YUNDEF;

    RETURN(env, iter);
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
