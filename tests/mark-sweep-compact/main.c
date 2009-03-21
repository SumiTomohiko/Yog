#include <stdlib.h>
#include <stdio.h>

#include <CUnit/CUnit.h>

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

    CU_cleanup_registry();

    return 0;
#undef ERROR
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
