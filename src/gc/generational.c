#ifdef TEST_GENERATIONAL
#   include <stdio.h>
#   include <stdlib.h>
#   include <CUnit/Basic.h>
#   include <CUnit/CUnit.h>
#endif
#include "yog/yog.h"
#include "yog/gc/generational.h"

void 
YogGenerational_minor_gc(YogEnv* env, YogGenerational* generational) 
{
    /* TODO */
}

void 
YogGenerational_initialize(YogEnv* env, YogGenerational* generational, BOOL stress, size_t young_heap_size, size_t old_chunk_size, size_t old_threshold, void* root, ChildrenKeeper root_keeper) 
{
    YogCopying* copying = &generational->copying;
    YogCopying_initialize(env, copying, stress, young_heap_size, NULL, NULL);

    YogMarkSweepCompact* msc = &generational->msc;
    YogMarkSweepCompact_initialize(env, msc, old_chunk_size, old_threshold, NULL, NULL);
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

#define CREATE_TEST(name, root, root_keeper) \
    static void \
    name() \
    { \
        YogVm vm; \
        YogEnv env; \
        env.vm = &vm; \
        YogGenerational_initialize(&env, &vm.gc.generational, FALSE, HEAP_SIZE, CHUNK_SIZE, THRESHOLD, root, root_keeper); \
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
