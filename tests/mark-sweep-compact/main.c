#include <stdlib.h>
#include <stdio.h>

#include <CUnit/Basic.h>
#include <CUnit/CUnit.h>

#include "yog/gc/mark-sweep-compact.h"

#define CHUNK_SIZE  (1 * 1024 * 1024)
#define THRESHOLD   (512 * 1024)

#define CREATE_TEST(name) \
    static void \
    name() \
    { \
        YogMarkSweepCompact msc; \
        YogMarkSweepCompact_initialize(&msc, CHUNK_SIZE, THRESHOLD); \
        \
        test_##name(&msc); \
        \
        YogMarkSweepCompact_finalize(&msc); \
    }

static void 
test_alloc1(YogMarkSweepCompact* msc) 
{
    void* ptr = YogMarkSweepCompact_alloc(msc, NULL, NULL, 0);
    CU_ASSERT_PTR_NOT_NULL(ptr);
}

CREATE_TEST(alloc1);

static void 
test_assign_page1(YogMarkSweepCompact* msc) 
{
    YogMarkSweepCompact_alloc(msc, NULL, NULL, 0);
    CU_ASSERT_PTR_NOT_NULL(msc->pages[0]);
}

CREATE_TEST(assign_page1);

static void 
test_use_up_page1(YogMarkSweepCompact* msc) 
{
    unsigned int obj_num = 63;
    unsigned int i;
    for (i = 0; i < obj_num; i++) {
        YogMarkSweepCompact_alloc(msc, NULL, NULL, 0);
    }
    CU_ASSERT_PTR_NULL(msc->pages[0]);
}

CREATE_TEST(use_up_page1);

static void 
test_gc1(YogMarkSweepCompact* msc) 
{
    YogMarkSweepCompact_alloc(msc, NULL, NULL, 0);
    YogMarkSweepCompact_gc(msc);
    CU_ASSERT_PTR_NULL(msc->pages[0]);
}

CREATE_TEST(gc1);

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

    CU_basic_set_mode(CU_BRM_VERBOSE);
    CU_basic_run_tests();

    CU_cleanup_registry();

    return 0;
#undef ADD_TEST
#undef ERROR
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
