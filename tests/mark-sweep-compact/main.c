#include <stdlib.h>
#include <stdio.h>
#include <CUnit/Basic.h>
#include <CUnit/CUnit.h>
#include "yog/gc/mark-sweep-compact.h"

#define CHUNK_SIZE  (1 * 1024 * 1024)
#define THRESHOLD   CHUNK_SIZE

#define CREATE_TEST(name, root, keeper) \
    static void \
    name() \
    { \
        YogMarkSweepCompact msc; \
        YogMarkSweepCompact_initialize(NULL, &msc, CHUNK_SIZE, THRESHOLD, root, keeper); \
        \
        test_##name(&msc); \
        \
        YogMarkSweepCompact_finalize(NULL, &msc); \
    }

static void 
test_alloc1(YogMarkSweepCompact* msc) 
{
    void* ptr = YogMarkSweepCompact_alloc(NULL, msc, NULL, NULL, 0);
    CU_ASSERT_PTR_NOT_NULL(ptr);
}

CREATE_TEST(alloc1, NULL, NULL);

static void 
test_assign_page1(YogMarkSweepCompact* msc) 
{
    YogMarkSweepCompact_alloc(NULL, msc, NULL, NULL, 0);
    CU_ASSERT_PTR_NOT_NULL(msc->pages[0]);
}

CREATE_TEST(assign_page1, NULL, NULL);

static void 
test_use_up_page1(YogMarkSweepCompact* msc) 
{
    unsigned int obj_num = 63;
    unsigned int i;
    for (i = 0; i < obj_num; i++) {
        YogMarkSweepCompact_alloc(NULL, msc, NULL, NULL, 0);
    }
    CU_ASSERT_PTR_NULL(msc->pages[0]);
}

CREATE_TEST(use_up_page1, NULL, NULL);

static void 
gc1_keeper(YogEnv* env, void* ptr, ObjectKeeper keeper) 
{
}

static void 
test_gc1(YogMarkSweepCompact* msc) 
{
    YogMarkSweepCompact_alloc(NULL, msc, NULL, NULL, 0);
    YogMarkSweepCompact_gc(NULL, msc);
    CU_ASSERT_PTR_NULL(msc->pages[0]);
}

CREATE_TEST(gc1, NULL, gc1_keeper);

static void* gc2_ptr = NULL;

static void 
gc2_keeper(YogEnv* env, void* ptr, ObjectKeeper keeper) 
{
    gc2_ptr = (*keeper)(env, gc2_ptr);
}

static void 
test_gc2(YogMarkSweepCompact* msc) 
{
    gc2_ptr = YogMarkSweepCompact_alloc(NULL, msc, NULL, NULL, 0);
    void* gc2_ptr_old = gc2_ptr;
    YogMarkSweepCompact_gc(NULL, msc);
    CU_ASSERT_PTR_EQUAL(gc2_ptr, gc2_ptr_old);
}

CREATE_TEST(gc2, NULL, gc2_keeper);

static void* gc3_ptr1 = NULL;
static void* gc3_ptr2 = NULL;

static void 
gc3_keeper(YogEnv* env, void* ptr, ObjectKeeper keeper) 
{
    gc3_ptr2 = (*keeper)(env, gc3_ptr2);
}

static void 
test_gc3(YogMarkSweepCompact* msc) 
{
    gc3_ptr1 = YogMarkSweepCompact_alloc(NULL, msc, NULL, NULL, 0);
    gc3_ptr2 = YogMarkSweepCompact_alloc(NULL, msc, NULL, NULL, 0);
    YogMarkSweepCompact_gc(NULL, msc);
    CU_ASSERT_PTR_EQUAL(gc3_ptr2, gc3_ptr1);
}

CREATE_TEST(gc3, NULL, gc3_keeper);

int 
main(int argc, const char* argv[]) 
{
#define ERROR(...)  do { \
    fprintf(stderr, __VA_ARGS__); \
    exit(-1); \
} while (0)
    if (CU_initialize_registry() != CUE_SUCCESS) {
        ERROR("failed CU_initialize_registry");
    }

    CU_pSuite suite = CU_add_suite("mark-sweep-compact", NULL, NULL);
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
    ADD_TEST(assign_page1);
    ADD_TEST(use_up_page1);
    ADD_TEST(gc1);
    ADD_TEST(gc2);
    ADD_TEST(gc3);
#undef ADD_TEST

    CU_basic_set_mode(CU_BRM_VERBOSE);
    CU_basic_run_tests();

    CU_cleanup_registry();

    return 0;
#undef ERROR
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
