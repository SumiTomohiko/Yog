#include "yog/yog.h"

int main(int argc, char* argv[]) 
{
#define INIT_HEAP_SIZE  (1)
    YogVm* vm = YogVm_new(INIT_HEAP_SIZE);
#undef INIT_HEAP_SIZE
    YogEnv e = { vm };
    YogVm_alloc_obj(&e, e.vm, OBJ_BUFFER, 1024);

    return 0;
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
