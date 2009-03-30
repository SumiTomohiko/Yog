#include <string.h>
#include "yog/yog.h"
#include "yog/gc/generational.h"
#ifdef TEST_GENERATIONAL
#   include <stdio.h>
#   include <stdlib.h>
#   include <CUnit/Basic.h>
#   include <CUnit/CUnit.h>
#endif

static void* 
copy_young_object(YogEnv* env, void* ptr, ObjectKeeper obj_keeper)
{
    YogGenerational* gen = &env->vm->gc.generational;
    YogCopyingHeader* header = (YogCopyingHeader*)ptr - 1;
    header->servive_num++;
    if (header->servive_num < gen->tenure) {
        return YogCopying_copy(env, &gen->copying, ptr);
    }
    else {
        YogMarkSweepCompact* msc = &gen->msc;
        ChildrenKeeper keeper = header->keeper;
        Finalizer finalizer = header->finalizer;
        size_t size = header->size - sizeof(YogCopyingHeader);
        void* p = YogMarkSweepCompact_alloc(env, msc, keeper, finalizer, size);
        memcpy(p, ptr, size);
        header->forwarding_addr = p;
        YogMarkSweepCompact_mark_recursively(env, p, obj_keeper);
        return p;
    }
}

static void* 
major_gc_keep_object(YogEnv* env, void* ptr) 
{
    if (ptr == NULL) {
        return NULL;
    }

    if (!IS_YOUNG(env, PTR2VAL(ptr))) {
        return YogMarkSweepCompact_mark_recursively(env, ptr, major_gc_keep_object);
    }
    else {
        return copy_young_object(env, ptr, major_gc_keep_object);
    }
}

static void* 
update_pointer(YogEnv* env, void* ptr) 
{
    if (ptr == NULL) {
        return NULL;
    }

    void* forwarding_addr;
    ChildrenKeeper keeper;
    if (IS_YOUNG(env, PTR2VAL(ptr))) {
        YogCopyingHeader* header = (YogCopyingHeader*)ptr - 1;
        keeper = header->keeper;
        forwarding_addr = ptr;
    }
    else {
        YogMarkSweepCompactHeader* header = (YogMarkSweepCompactHeader*)ptr - 1;
        keeper = header->keeper;
        forwarding_addr = header->forwarding_addr;
    }
    if (keeper != NULL) {
        (*keeper)(env, ptr, update_pointer);
    }

    return forwarding_addr;
}

void 
YogGenerational_major_gc(YogEnv* env, YogGenerational* generational) 
{
    YogMarkSweepCompact* msc = &generational->msc;
    msc->in_gc = TRUE;
    YogMarkSweepCompact_unmark_all(env, msc);

    YogCopying_do_gc(env, &generational->copying, major_gc_keep_object);
    YogMarkSweepCompact_delete_garbage(env, msc);
    YogMarkSweepCompact_do_compaction(env, msc, update_pointer);

    msc->in_gc = FALSE;
}

static void* 
minor_gc_keep_object(YogEnv* env, void* ptr) 
{
    if (ptr == NULL) {
        return NULL;
    }
    if (!IS_YOUNG(env, PTR2VAL(ptr))) {
        return ptr;
    }

    return copy_young_object(env, ptr, minor_gc_keep_object);
}

void 
YogGenerational_minor_gc(YogEnv* env, YogGenerational* generational) 
{
    YogMarkSweepCompact* msc = &generational->msc;
    msc->in_gc = TRUE;

    YogCopying_do_gc(env, &generational->copying, minor_gc_keep_object);

    msc->in_gc = FALSE;
}

void 
YogGenerational_initialize(YogEnv* env, YogGenerational* generational, BOOL stress, size_t young_heap_size, size_t old_chunk_size, size_t old_threshold, unsigned int tenure, void* root, ChildrenKeeper root_keeper) 
{
    generational->err = ERR_GEN_NONE;

    YogCopying* copying = &generational->copying;
    YogCopying_initialize(env, copying, stress, young_heap_size, root, root_keeper);

    YogMarkSweepCompact* msc = &generational->msc;
    YogMarkSweepCompact_initialize(env, msc, old_chunk_size, old_threshold, root, root_keeper);

    generational->tenure = tenure;
}

void 
YogGenerational_finalize(YogEnv* env, YogGenerational* generational) 
{
    generational->err = ERR_GEN_NONE;

    YogMarkSweepCompact* msc = &generational->msc;
    YogMarkSweepCompact_finalize(env, msc);

    YogCopying* copying = &generational->copying;
    YogCopying_finalize(env, copying);
}

void* 
YogGenerational_alloc(YogEnv* env, YogGenerational* generational, ChildrenKeeper keeper, Finalizer finalizer, size_t size)
{
    YogCopying* copying = &generational->copying;
    void* ptr = YogCopying_alloc(env, copying, keeper, finalizer, size);
    if (ptr != NULL) {
        return ptr;
    }

    YogMarkSweepCompact* msc = &generational->msc;
    ptr = YogMarkSweepCompact_alloc(env, msc, keeper, finalizer, size);
    if (ptr != NULL) {
        return ptr;
    }

    unsigned int err;
    switch (msc->err) {
    case ERR_MSC_MMAP:
        err = ERR_GEN_MMAP;
        break;
    case ERR_MSC_MUNMAP: 
        err = ERR_GEN_MUNMAP;
        break;
    case ERR_MSC_MALLOC: 
        err = ERR_GEN_MALLOC;
        break;
    default:
        err = ERR_GEN_UNKNOWN;
        break;
    }
    generational->err = err;

    return NULL;
}

#ifdef TEST_GENERATIONAL
#define CHUNK_SIZE  (1 * 1024 * 1024)
#define THRESHOLD   CHUNK_SIZE
#define HEAP_SIZE   (1 * 1024 * 1024)
#define TENURE      32

#define CREATE_TEST(name, root, root_keeper) \
    static void \
    name() \
    { \
        YogVm vm; \
        YogEnv env; \
        env.vm = &vm; \
        YogGenerational_initialize(&env, &vm.gc.generational, FALSE, HEAP_SIZE, CHUNK_SIZE, THRESHOLD, TENURE, root, root_keeper); \
        \
        test_##name(&env); \
        \
        YogGenerational_finalize(&env, &vm.gc.generational); \
    }

static void 
test_alloc1(YogEnv* env) 
{
    YogGenerational* gen = &env->vm->gc.generational;
    void* ptr = YogGenerational_alloc(env, gen, NULL, NULL, 0);
    CU_ASSERT_PTR_NOT_NULL(ptr);
}

CREATE_TEST(alloc1, NULL, NULL);

static void 
dummy_keeper(YogEnv* env, void* ptr, ObjectKeeper keeper) 
{
}

static void 
test_alloc2(YogEnv* env) 
{
    YogGenerational* gen = &env->vm->gc.generational;
    void* ptr = YogGenerational_alloc(env, gen, NULL, NULL, HEAP_SIZE + 1);
    CU_ASSERT_PTR_NOT_NULL(ptr);
}

CREATE_TEST(alloc2, NULL, NULL);

static unsigned char* minor_gc1_ptr = NULL;

static void
minor_gc1_keep_children(YogEnv* env, void* ptr, ObjectKeeper keeper) 
{
    minor_gc1_ptr = (*keeper)(env, minor_gc1_ptr);
}

static void 
test_minor_gc1(YogEnv* env) 
{
    YogGenerational* gen = &env->vm->gc.generational;
    minor_gc1_ptr = YogGenerational_alloc(env, gen, NULL, NULL, 0);
    unsigned char* minor_gc1_ptr_old = minor_gc1_ptr;

    YogGenerational_minor_gc(env, gen);

    YogCopyingHeap* heap = gen->copying.active_heap;
    CU_ASSERT_TRUE((heap->items <= minor_gc1_ptr) && (minor_gc1_ptr <= heap->items + heap->size));
    CU_ASSERT_PTR_NOT_EQUAL(minor_gc1_ptr, minor_gc1_ptr_old);
}

CREATE_TEST(minor_gc1, NULL, minor_gc1_keep_children);

static unsigned char* minor_gc2_ptr = NULL;

static void
minor_gc2_keep_children(YogEnv* env, void* ptr, ObjectKeeper keeper) 
{
    minor_gc2_ptr = (*keeper)(env, minor_gc2_ptr);
}

static void 
oldify(YogEnv* env, YogGenerational* gen, void* ptr) 
{
    YogCopyingHeader* header = (YogCopyingHeader*)ptr - 1;
    header->servive_num = gen->tenure - 1;
}

static void 
test_minor_gc2(YogEnv* env) 
{
    YogGenerational* gen = &env->vm->gc.generational;
    minor_gc2_ptr = YogGenerational_alloc(env, gen, NULL, NULL, 0);
    oldify(env, gen, minor_gc2_ptr);

    YogGenerational_minor_gc(env, gen);

    YogCopyingHeap* heap = gen->copying.active_heap;
    CU_ASSERT_TRUE((minor_gc2_ptr < heap->items) || (heap->items + heap->size < minor_gc2_ptr));
}

CREATE_TEST(minor_gc2, NULL, minor_gc2_keep_children);

static unsigned char* major_gc1_ptr = NULL;

static void
major_gc1_keep_children(YogEnv* env, void* ptr, ObjectKeeper keeper) 
{
    major_gc1_ptr = (*keeper)(env, major_gc1_ptr);
}

static void 
test_major_gc1(YogEnv* env) 
{
    YogGenerational* gen = &env->vm->gc.generational;
    major_gc1_ptr = YogGenerational_alloc(env, gen, NULL, NULL, 0);
    unsigned char* major_gc1_ptr_old = major_gc1_ptr;

    YogGenerational_major_gc(env, gen);

    YogCopyingHeap* heap = gen->copying.active_heap;
    CU_ASSERT_TRUE((heap->items <= major_gc1_ptr) && (major_gc1_ptr <= heap->items + heap->size));
    CU_ASSERT_PTR_NOT_EQUAL(major_gc1_ptr, major_gc1_ptr_old);
}

CREATE_TEST(major_gc1, NULL, major_gc1_keep_children);

static unsigned char* major_gc2_ptr = NULL;

static void
major_gc2_keep_children(YogEnv* env, void* ptr, ObjectKeeper keeper) 
{
    major_gc2_ptr = (*keeper)(env, major_gc2_ptr);
}

static void 
test_major_gc2(YogEnv* env) 
{
    YogGenerational* gen = &env->vm->gc.generational;
    major_gc2_ptr = YogGenerational_alloc(env, gen, NULL, NULL, 0);
    oldify(env, gen, major_gc2_ptr);

    YogGenerational_major_gc(env, gen);

    YogCopyingHeap* heap = gen->copying.active_heap;
    CU_ASSERT_TRUE((major_gc2_ptr < heap->items) || (heap->items + heap->size < major_gc2_ptr));
}

CREATE_TEST(major_gc2, NULL, major_gc2_keep_children);

static void
major_gc3_ptr_keep_children(YogEnv* env, void* ptr, ObjectKeeper keeper) 
{
    *(void**)ptr = (*keeper)(env, *(void**)ptr);
}

static unsigned char* major_gc3_ptr = NULL;

static void
major_gc3_keep_children(YogEnv* env, void* ptr, ObjectKeeper keeper) 
{
    major_gc3_ptr = (*keeper)(env, major_gc3_ptr);
}

static void 
test_major_gc3(YogEnv* env) 
{
    YogGenerational* gen = &env->vm->gc.generational;
    major_gc3_ptr = YogGenerational_alloc(env, gen, major_gc3_ptr_keep_children, NULL, sizeof(void*));
    *(void**)major_gc3_ptr = YogGenerational_alloc(env, gen, NULL, NULL, 0);
    unsigned char* major_gc3_ptr_old = *(void**)major_gc3_ptr;

    YogGenerational_major_gc(env, gen);

    YogCopyingHeap* heap = gen->copying.active_heap;
    CU_ASSERT_TRUE((heap->items <= (unsigned char*)(*(void**)major_gc3_ptr)) && ((unsigned char*)(*(void**)major_gc3_ptr) <= heap->items + heap->size));
    CU_ASSERT_PTR_NOT_EQUAL(major_gc3_ptr, major_gc3_ptr_old);
}

CREATE_TEST(major_gc3, NULL, major_gc3_keep_children);

static BOOL finalize1_flag = FALSE;

static void 
finalize1_finalize(YogEnv* env, void* ptr) 
{
    finalize1_flag = TRUE;
}

static void 
test_finalize1(YogEnv* env) 
{
    YogGenerational* gen = &env->vm->gc.generational;
    YogGenerational_alloc(env, gen, NULL, finalize1_finalize, 0);
    YogGenerational_minor_gc(env, gen);
    CU_ASSERT_TRUE(finalize1_flag);
}

CREATE_TEST(finalize1, NULL, dummy_keeper);

static BOOL finalize2_flag = FALSE;
static void* finalize2_ptr = NULL;

static void 
finalize2_finalize(YogEnv* env, void* ptr) 
{
    finalize2_flag = TRUE;
}

static void 
finalize2_root_keeper(YogEnv* env, void* ptr, ObjectKeeper keeper) 
{
    finalize2_ptr = (*keeper)(env, finalize2_ptr);
}

static void 
test_finalize2(YogEnv* env) 
{
    YogGenerational* gen = &env->vm->gc.generational;
    finalize2_ptr = YogGenerational_alloc(env, gen, NULL, finalize2_finalize, 0);
    oldify(env, gen, finalize2_ptr);
    YogGenerational_minor_gc(env, gen);

    finalize2_ptr = NULL;
    YogGenerational_major_gc(env, gen);

    CU_ASSERT_TRUE(finalize2_flag);
}

CREATE_TEST(finalize2, NULL, finalize2_root_keeper);

YogThread thread;

static void 
test_ref_tbl1(YogEnv* env) 
{
    YogThread_initialize(env, &thread);
    env->th = &thread;

    YogGenerational* gen = &env->vm->gc.generational;
    void* ptr = YogGenerational_alloc(env, gen, NULL, NULL, sizeof(int));
    *(int*)ptr = 42;
    ADD_REF(env, ptr);

    void* ptr_old = ptr;

    YogGenerational_minor_gc(env, gen);

    CU_ASSERT_PTR_NOT_EQUAL(ptr, ptr_old);
    CU_ASSERT_EQUAL(*(int*)ptr, 42);
}

CREATE_TEST(ref_tbl1, &thread, YogThread_keep_children);

#define PRIVATE

PRIVATE int 
main(int argc, const char* argv[]) 
{
#define ERROR(...)  do { \
    fprintf(stderr, __VA_ARGS__); \
    exit(-1); \
} while (0)
    if (CU_initialize_registry() != CUE_SUCCESS) {
        ERROR("failed CU_initialize_registry");
    }

    CU_pSuite suite = CU_add_suite("generational", NULL, NULL);
    if (suite == NULL) {
        ERROR("failed CU_add_suite");
    }

#define ADD_TEST(name)  do { \
    CU_pTest test = CU_add_test(suite, #name, name); \
    if (test == NULL) { \
        ERROR("failed CU_add_test %s", #name); \
    } \
} while (0)
    ADD_TEST(alloc1);
    ADD_TEST(alloc2);
    ADD_TEST(minor_gc1);
    ADD_TEST(minor_gc2);
    ADD_TEST(major_gc1);
    ADD_TEST(major_gc2);
    ADD_TEST(major_gc3);
    ADD_TEST(finalize1);
    ADD_TEST(finalize2);
    ADD_TEST(ref_tbl1);
#undef ADD_TEST

    CU_basic_set_mode(CU_BRM_VERBOSE);
    CU_basic_run_tests();

    CU_cleanup_registry();

    return 0;
#undef ERROR
}
#endif

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
