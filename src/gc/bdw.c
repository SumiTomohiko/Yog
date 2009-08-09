#include <stddef.h>
#include <string.h>
#include "gc.h"
#include "yog/thread.h"
#include "yog/yog.h"

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

static void
initialize_memory(void* ptr, size_t size)
{
    memset(ptr, 0xcb, size);
}

void*
YogBDW_alloc(YogEnv* env, YogBDW* bdw, ChildrenKeeper keeper, Finalizer finalizer, size_t size)
{
    if (env->vm->gc_stress) {
        GC_gcollect();
    }

    uint_t total_size = size + sizeof(BDWHeader);
    BDWHeader* header = GC_MALLOC(total_size);
    initialize_memory(header, total_size);

    header->finalizer = finalizer;

    if (finalizer != NULL) {
        GC_REGISTER_FINALIZER(header, bdw_finalizer, env, 0, 0);
    }

    return header + 1;
}

void
YogBDW_initialize(YogEnv* env, YogBDW* bdw)
{
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
