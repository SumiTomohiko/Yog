#include <sys/types.h>
#include "yog/yog.h"
#include "yog/vm.h"

YogVal 
YogGC_allocate(YogEnv* env, ChildrenKeeper keeper, Finalizer finalizer, size_t size) 
{
    YogVm* vm = env->vm;
    vm->gc_stat.num_alloc++;

    void* ptr = (*vm->alloc_mem)(env, vm, keeper, finalizer, size);

    if (ptr != NULL) {
        return PTR2VAL(ptr);
    }
    else {
        return YNIL;
    }
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
