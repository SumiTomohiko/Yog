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
major_gc_keep_object(YogEnv* env, void* ptr) 
{
    /* TODO */
    return NULL;
}

void 
YogGenerational_major_gc(YogEnv* env, YogGenerational* generational) 
{
    YogCopying_do_gc(env, &generational->copying, major_gc_keep_object);
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
        size_t size = header->size;
        void* p = YogMarkSweepCompact_alloc(env, msc, keeper, finalizer, size);
        memcpy(p, ptr, size);
        return p;
    }
}

void 
YogGenerational_minor_gc(YogEnv* env, YogGenerational* generational) 
{
    YogCopying_do_gc(env, &generational->copying, minor_gc_keep_object);
}

void 
YogGenerational_initialize(YogEnv* env, YogGenerational* generational, BOOL stress, size_t young_heap_size, size_t old_chunk_size, size_t old_threshold, unsigned int tenure, void* root, ChildrenKeeper root_keeper) 
{
    generational->err = ERR_GEN_NONE;

    YogCopying* copying = &generational->copying;
    YogCopying_initialize(env, copying, stress, young_heap_size, root, root_keeper);

    YogMarkSweepCompact* msc = &generational->msc;
    YogMarkSweepCompact_initialize(env, msc, old_chunk_size, old_threshold, NULL, NULL);

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

#if 0
static void 
dummy_keeper(YogEnv* env, void* ptr, ObjectKeeper keeper) 
{
}
#endif

static void 
test_alloc2(YogEnv* env) 
{
    YogGenerational* gen = &env->vm->gc.generational;
    void* ptr = YogGenerational_alloc(env, gen, NULL, NULL, HEAP_SIZE + 1);
    CU_ASSERT_PTR_NOT_NULL(ptr);
}

CREATE_TEST(alloc2, NULL, NULL);

static unsigned char* minor_gc1_ptr;

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

static unsigned char* minor_gc2_ptr;

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
