#ifdef TEST
#   include <stdio.h>
#   include <stdlib.h>
#   include <CUnit/Basic.h>
#   include <CUnit/CUnit.h>
#endif

#ifdef TEST
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
    /* TODO */
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
