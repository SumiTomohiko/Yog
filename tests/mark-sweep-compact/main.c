#include <stdlib.h>
#include <stdio.h>

#include <CUnit/Basic.h>
#include <CUnit/CUnit.h>

#include "yog/gc/mark-sweep-compact.h"

#define CREATE_TEST(name) \
    static void \
    name() \
    { \
        YogMarkSweepCompact msc; \
        YogMarkSweepCompact_initialize(&msc); \
        \
        test_##name(&msc); \
        \
        YogMarkSweepCompact_finalize(&msc); \
    }

static void 
test_alloc1(YogMarkSweepCompact* msc) 
{
}

CREATE_TEST(alloc1);

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
