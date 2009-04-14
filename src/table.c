/* This is a public domain general purpose hash table package written by Peter Moore @ UCB. */

/* static        char        sccsid[] = "@(#) st.c 5.1 89/12/14 Crucible"; */

#include "config.h"
#if 0
#   include "defines.h"
#endif
#include <stdio.h>
#if defined(HAVE_STDLIB_H)
#include <stdlib.h>
#endif
#include <string.h>
#include "yog/env.h"
#include "yog/error.h"
#include "yog/st.h"
#include "yog/string.h"
#include "yog/thread.h"
#include "yog/vm.h"
#include "yog/yog.h"

#if 0
#   define DEBUG(x)     x
#else
#   define DEBUG(x)
#endif

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

#define TABLE_ENTRY_TOP(table, i)   (PTR_AS(YogTableEntryArray, PTR_AS(YogTable, table)->bins)->items[(i)])

static void 
keep_bins_children(YogEnv* env, void* ptr, ObjectKeeper keeper)
{
    YogTableEntryArray* array = ptr;
    unsigned int size = array->size;
    unsigned int i = 0;
    for (i = 0; i < size; i++) {
        array->items[i] = YogVal_keep(env, array->items[i], keeper);
    }
}

static YogVal 
alloc_bins(YogEnv* env, int size) 
{
    YogVal array = ALLOC_OBJ_ITEM(env, keep_bins_children, NULL, YogTableEntryArray, size, YogVal);

    PTR_AS(YogTableEntryArray, array)->size = size;
    unsigned int i = 0;
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
rehash(YogEnv* env, YogVal table)
{
    SAVE_LOCALS(env);
    PUSH_LOCAL(env, table);

    int old_num_bins = PTR_AS(YogTable, table)->num_bins;
    int new_num_bins = new_size(old_num_bins + 1);
    YogVal new_bins = alloc_bins(env, new_num_bins);

    int i = 0;
    for(i = 0; i < old_num_bins; i++) {
        YogVal ptr = TABLE_ENTRY_TOP(table, i);
        while (VAL2PTR(ptr) != NULL) {
            YogVal next = PTR_AS(YogTableEntry, ptr)->next;
            unsigned int hash_val = PTR_AS(YogTableEntry, ptr)->hash % new_num_bins;
            MODIFY(env, PTR_AS(YogTableEntry, ptr)->next, PTR_AS(YogTableEntryArray, new_bins)->items[hash_val]);
            PTR_AS(YogTableEntryArray, new_bins)->items[hash_val] = ptr;
            ptr = next;
        }
    }

    PTR_AS(YogTable, table)->num_bins = new_num_bins;
    MODIFY(env, PTR_AS(YogTable, table)->bins, new_bins);

    RETURN_VOID(env);
}

#define EQUAL(env, table, x, y) ((*PTR_AS(YogTable, table)->type->compare)((env), (x), (y)) == 0)

#define do_hash(env, table, key) (unsigned int)(*PTR_AS(YogTable, table)->type->hash)((env), (key))
#define do_hash_bin(env, table, key) (do_hash(env, table, key) % PTR_AS(YogTable, table)->num_bins)

#if defined(HASH_LOG)
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
keep_table_children(YogEnv* env, void* ptr, ObjectKeeper keeper)
{
    YogTable* tbl = ptr;
    tbl->bins = YogVal_keep(env, tbl->bins, keeper);
}

static YogVal 
alloc_table(YogEnv* env) 
{
    YogVal tbl = ALLOC_OBJ(env, keep_table_children, NULL, YogTable);
    PTR_AS(YogTable, tbl)->type = NULL;
    PTR_AS(YogTable, tbl)->num_bins = 0;
    PTR_AS(YogTable, tbl)->num_entries = 0;
    PTR_AS(YogTable, tbl)->bins = YNIL;

    return tbl;
}

static YogVal 
st_init_table_with_size(YogEnv* env, YogHashType* type, int size)
{
    SAVE_LOCALS(env);

    YogVal tbl = YUNDEF;
    PUSH_LOCAL(env, tbl);

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
    YogVal bins = alloc_bins(env, size);
    MODIFY(env, PTR_AS(YogTable, tbl)->bins, bins);

    RETURN(env, tbl);
}

static YogVal 
st_init_table(YogEnv* env, YogHashType* type)
{
    return st_init_table_with_size(env, type, 0);
}

#define PTR_NOT_EQUAL(env, table, ptr, hash_val, key) \
    (IS_PTR((ptr)) && (PTR_AS(YogTableEntry, (ptr))->hash != (hash_val) || !EQUAL((env), (table), (key), PTR_AS(YogTableEntry, (ptr))->key)))

#if defined(HASH_LOG)
#define COLLISION collision++
#else
#define COLLISION
#endif

static void 
find_entry(YogEnv* env, YogVal table, YogVal* ptr, unsigned int hash_val, unsigned int* bin_pos, YogVal key) 
{
    *bin_pos = hash_val % PTR_AS(YogTable, table)->num_bins;
    *ptr = TABLE_ENTRY_TOP(table, *bin_pos);
    if (PTR_NOT_EQUAL(env, table, *ptr, hash_val, key)) {
        COLLISION;
        while (PTR_NOT_EQUAL(env, table, PTR_AS(YogTableEntry, (*ptr))->next, hash_val, key)) {
            *ptr = PTR_AS(YogTableEntry, (*ptr))->next;
        }
        *ptr = PTR_AS(YogTableEntry, (*ptr))->next;
    }
}

BOOL
YogTable_lookup(YogEnv* env, YogVal table, YogVal key, YogVal* value) 
{
    unsigned int hash_val = do_hash(env, table, key);

    unsigned int bin_pos = 0;
    YogVal ptr = YNIL;
    find_entry(env, table, &ptr, hash_val, &bin_pos, key);

    if (!IS_PTR(ptr)) {
        return FALSE;
    }
    else {
        if (value != NULL) {
            *value = PTR_AS(YogTableEntry, ptr)->record;
        }
        return TRUE;
    }
}

static void 
keep_entry_children(YogEnv* env, void* ptr, ObjectKeeper keeper)
{
    YogTableEntry* entry = ptr;
#define KEEP(member)    entry->member = YogVal_keep(env, entry->member, keeper)
    KEEP(key);
    KEEP(record);
    KEEP(next);
#undef KEEP
}

static YogVal 
alloc_entry(YogEnv* env)
{
    YogVal entry = ALLOC_OBJ(env, keep_entry_children, NULL, YogTableEntry);
    PTR_AS(YogTableEntry, entry)->hash = 0;
    PTR_AS(YogTableEntry, entry)->key = YUNDEF;
    PTR_AS(YogTableEntry, entry)->record = YUNDEF;
    PTR_AS(YogTableEntry, entry)->next = YUNDEF;

    return entry;
}

static void 
add_direct(YogEnv* env, YogVal table, YogVal key, YogVal value, unsigned int hash_val, unsigned int bin_pos) 
{
    SAVE_LOCALS(env);
    PUSH_LOCALS3(env, table, key, value);

    if (ST_DEFAULT_MAX_DENSITY < PTR_AS(YogTable, table)->num_entries / (PTR_AS(YogTable, table)->num_bins)) {
        rehash(env, table);
        bin_pos = hash_val % PTR_AS(YogTable, table)->num_bins;
    }

    YogVal entry = alloc_entry(env);

    PTR_AS(YogTableEntry, entry)->hash = hash_val;
    MODIFY(env, PTR_AS(YogTableEntry, entry)->key, key);
    MODIFY(env, PTR_AS(YogTableEntry, entry)->record, value);
    MODIFY(env, PTR_AS(YogTableEntry, entry)->next, TABLE_ENTRY_TOP(table, bin_pos));

    MODIFY(env, TABLE_ENTRY_TOP(table, bin_pos), entry);
    PTR_AS(YogTable, table)->num_entries++;

    RETURN_VOID(env);
}

BOOL
YogTable_insert(YogEnv* env, YogVal table, YogVal key, YogVal value) 
{
    unsigned int hash_val = do_hash(env, table, key);

    unsigned int bin_pos = 0;
    YogVal ptr = YNIL;
    find_entry(env, table, &ptr, hash_val, &bin_pos, key);

    if (!IS_PTR(ptr)) {
        add_direct(env, table, key, value, hash_val, bin_pos);
        return FALSE;
    }
    else {
        MODIFY(env, PTR_AS(YogTableEntry, ptr)->record, value);
        return TRUE;
    }
}

void
YogTable_add_direct(YogEnv* env, YogVal table, YogVal key, YogVal value) 
{
    SAVE_LOCALS(env);
    PUSH_LOCALS3(env, table, key, value);

    unsigned int hash_val = do_hash(env, table, key);
    unsigned int bin_pos = hash_val % PTR_AS(YogTable, table)->num_bins;
    add_direct(env, table, key, value, hash_val, bin_pos);

    RETURN_VOID(env);
}

BOOL
YogTable_delete(YogEnv* env, YogVal table, YogVal* key, YogVal* value) 
{
    unsigned int hash_val = do_hash_bin(env, table, *key);
    YogVal ptr = TABLE_ENTRY_TOP(table, hash_val);

    if (!IS_PTR(ptr)) {
        if (value != NULL) {
            *value = YNIL;
        }
        return FALSE;
    }

    if (EQUAL(env, table, *key, PTR_AS(YogTableEntry, ptr)->key)) {
        TABLE_ENTRY_TOP(table, hash_val) = PTR_AS(YogTableEntry, ptr)->next;
        PTR_AS(YogTable, table)->num_entries--;
        if (value != NULL) {
            *value = PTR_AS(YogTableEntry, ptr)->record;
        }
        *key = PTR_AS(YogTableEntry, ptr)->key;
        return TRUE;
    }

    for(; IS_PTR(PTR_AS(YogTableEntry, ptr)->next); ptr = PTR_AS(YogTableEntry, ptr)->next) {
        YogVal next = PTR_AS(YogTableEntry, ptr)->next;
        if (EQUAL(env, table, PTR_AS(YogTableEntry, next)->key, *key)) {
            YogVal tmp = next;
            MODIFY(env, PTR_AS(YogTableEntry, ptr)->next, PTR_AS(YogTableEntry, next)->next);
            PTR_AS(YogTable, table)->num_entries--;
            if (value != NULL) {
                *value = PTR_AS(YogTableEntry, tmp)->record;
            }
            *key = PTR_AS(YogTableEntry, tmp)->key;
            return TRUE;
        }
    }

    return FALSE;
}

BOOL
YogTable_delete_safe(YogEnv* env, YogVal table, YogVal* key, YogVal* value, YogVal never)
{
    unsigned int hash_val = do_hash_bin(env, table, *key);
    YogVal ptr = TABLE_ENTRY_TOP(table, hash_val);

    if (!IS_PTR(ptr)) {
        if (value != NULL) {
            *value = YNIL;
        }
        return FALSE;
    }

    for(; IS_PTR(ptr); ptr = PTR_AS(YogTableEntry, ptr)->next) {
        YogVal ptr_key = PTR_AS(YogTableEntry, ptr)->key;
        if (!YogVal_equals_exact(env, ptr_key, never) && EQUAL(env, table, ptr_key, *key)) {
            PTR_AS(YogTable, table)->num_entries--;
            *key = ptr_key;
            if (value != NULL) {
                *value = PTR_AS(YogTableEntry, ptr)->record;
            }
            MODIFY(env, PTR_AS(YogTableEntry, ptr)->key, PTR_AS(YogTableEntry, ptr)->record = never);
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
YogTable_foreach(YogEnv* env, YogVal table, int (*func)(YogEnv*, YogVal, YogVal, YogVal*), YogVal* arg)
{
    SAVE_ARG(env, table);

    YogVal last = YUNDEF;
    YogVal ptr = YUNDEF;
    YogVal tmp = YUNDEF;
    PUSH_LOCALS3(env, last, ptr, tmp);

    int i = 0;
    for (i = 0; i < PTR_AS(YogTable, table)->num_bins; i++) {
        for (ptr = TABLE_ENTRY_TOP(table, i); IS_PTR(ptr);) {
            enum st_retval retval = (*func)(env, PTR_AS(YogTableEntry, ptr)->key, PTR_AS(YogTableEntry, ptr)->record, arg);
            switch (retval) {
                case ST_CHECK:        /* check if hash is modified during iteration */
                    tmp = PTR2VAL(NULL);
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
                        TABLE_ENTRY_TOP(table, i) = PTR_AS(YogTableEntry, ptr)->next;
                    }
                    else {
                        MODIFY(env, PTR_AS(YogTableEntry, last)->next, PTR_AS(YogTableEntry, ptr)->next);
                    }
                    ptr = PTR_AS(YogTableEntry, ptr)->next;
                    PTR_AS(YogTable, table)->num_entries--;
                    break;
            }
        }
    }

    RETURN(env, TRUE);
}

void
YogTable_cleanup_safe(YogEnv* env, YogVal table, YogVal* never)
{
    int num_entries = PTR_AS(YogTable, table)->num_entries;

    YogTable_foreach(env, table, delete_never, never);
    PTR_AS(YogTable, table)->num_entries = num_entries;
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

YogVal 
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
#if 0
    const char* s = VAL2STR(a);
    const char* t = VAL2STR(b);
    return strcmp(s, t);
#endif
}

static int
strhash(const char* string)
{
    register int c;

#if defined(HASH_ELFHASH)
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
#if 0
    const char* s = VAL2STR(key);
    return strhash(s);
#endif
}

static YogHashType type_string = {
    compare_string, 
    hash_string, 
};

YogVal 
YogTable_new_string_table(YogEnv* env) 
{
    return st_init_table(env, &type_string);
}

BOOL
YogTable_lookup_str(YogEnv* env, YogVal table, const char* key, YogVal* value) 
{
    YOG_ASSERT(env, PTR_AS(YogTable, table)->type == &type_string, "Table type must be type_string.");

    unsigned int hash_val = strhash(key);
    unsigned int bin_pos = hash_val % PTR_AS(YogTable, table)->num_bins;
    YogVal entry = TABLE_ENTRY_TOP(table, bin_pos);

#if 0
#define NOT_EQUAL_ENTRY ((entry != NULL) && ((entry->hash != hash_val) || (strcmp(VAL2STR(entry->key), key) != 0)))
#endif
#define NOT_EQUAL_ENTRY (IS_PTR(entry) && ((PTR_AS(YogTableEntry, entry)->hash != hash_val) || (strcmp(((YogCharArray*)VAL2PTR(PTR_AS(YogTableEntry, entry)->key))->items, key) != 0)))
    if (NOT_EQUAL_ENTRY) {
        COLLISION;
        do {
            entry = PTR_AS(YogTableEntry, entry)->next;
        } while (NOT_EQUAL_ENTRY);
    }
#undef EQUAL_ENTRY

    if (IS_PTR(entry)) {
        if (value != NULL) {
            *value = PTR_AS(YogTableEntry, entry)->record;
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

YogVal 
YogTable_new_val_table(YogEnv* env) 
{
    return st_init_table(env, &type_val);
}

int 
YogTable_size(YogEnv* env, YogVal table) 
{
    return PTR_AS(YogTable, table)->num_entries;
}

/**
 * TODO: commonize with YogCode_dump.
 */
static void 
print_val(YogEnv* env, YogVal val) 
{
    if (IS_UNDEF(val)) {
        printf("undef");
    }
    else if (IS_PTR(val)) {
        printf("%p", VAL2PTR(val));
    }
    else if (IS_INT(val)) {
        printf("%d", VAL2INT(val));
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
        printf(" :%s", YogVm_id2name(env, env->vm, VAL2ID(val)));
    }
    else {
        YOG_ASSERT(env, FALSE, "Unknown value type.");
    }
}

static int 
dump_callback(YogEnv* env, YogVal key, YogVal value, YogVal* arg) 
{
    printf("  ");
    print_val(env, key);
    printf(" => ");
    print_val(env, value);
    printf(", \n");

    return ST_CONTINUE;
}

static int
dump_string_callback(YogEnv* env, YogVal key, YogVal value, YogVal* arg) 
{
    printf("  \"%s\" => ", PTR_AS(YogCharArray, key)->items);
    print_val(env, value);
    printf(", \n");

    return ST_CONTINUE;
}

void 
YogTable_dump(YogEnv* env, YogVal table) 
{
    printf("{ \n");
    if (PTR_AS(YogTable, table)->type == &type_val) {
        YogTable_foreach(env, table, dump_callback, NULL);
    }
    else if (PTR_AS(YogTable, table)->type == &type_string) {
        YogTable_foreach(env, table, dump_string_callback, NULL);
    }
    printf("}\n");
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
