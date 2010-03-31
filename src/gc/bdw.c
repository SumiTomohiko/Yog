#include <stddef.h>
#include <string.h>
#include "gc.h"
#include "yog/gc.h"
#include "yog/vm.h"
#include "yog/yog.h"

struct BDW {
    struct YogHeap base;
};

typedef struct BDW BDW;

struct BDWHeader {
    Finalizer finalizer;
};

typedef struct BDWHeader BDWHeader;

static void
bdw_finalizer(void* obj, void* client_data)
{
    BDWHeader* header = obj;
    YogEnv* env = client_data;
    (*header->finalizer)(env, header + 1);
}

void*
YogBDW_alloc(YogEnv* env, YogHeap* heap, ChildrenKeeper keeper, Finalizer finalizer, size_t size)
{
    if (env->vm->gc_stress) {
        GC_gcollect();
    }

    uint_t total_size = size + sizeof(BDWHeader);
    BDWHeader* header = GC_MALLOC(total_size);
    YogGC_init_memory(env, header, total_size);

    header->finalizer = finalizer;

    if (finalizer != NULL) {
        GC_REGISTER_FINALIZER(header, bdw_finalizer, env, 0, 0);
    }

    return header + 1;
}

YogHeap*
YogBDW_new(YogEnv* env)
{
    BDW* heap = (BDW*)YogGC_malloc(env, sizeof(BDW));
    YogHeap_init(env, (YogHeap*)heap);
    return heap;
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
