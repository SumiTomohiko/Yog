#include "yog/yog.h"

int main(int argc, char* argv[]) 
{
#define INIT_HEAP_SIZE  (1)
    YogVm* vm = YogVm_new(INIT_HEAP_SIZE);
#undef INIT_HEAP_SIZE
    YogEnv env = { vm };
    YogVm_alloc_obj(&env, env.vm, OBJ_ARRAY, 1024);
    YogTable_new_symbol_table(&env);
    YogArray_new(&env);

    return 0;
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
