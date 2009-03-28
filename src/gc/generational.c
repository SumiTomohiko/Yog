#ifdef TEST
#   include <stdio.h>
#   include <stdlib.h>
#   include <CUnit/Basic.h>
#   include <CUnit/CUnit.h>
#endif
#include "yog/yog.h"
#include "yog/gc/generational.h"

void 
YogGenerational_initialize(YogEnv* env, YogGenerational* generational, BOOL stress, size_t young_heap_size, size_t old_chunk_size, size_t old_threshold, void* root, ObjectKeeper root_keeper) 
{
    /* TODO */
}

void 
YogGenerational_finalize(YogEnv* env, YogGenerational* generational) 
{
    /* TODO */
}

void* 
YogGenerational_alloc(YogEnv* env, YogGenerational* copying, ChildrenKeeper keeper, Finalizer finalizer, size_t size)
{
    /* TODO */
    return NULL;
}

#ifdef TEST
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
