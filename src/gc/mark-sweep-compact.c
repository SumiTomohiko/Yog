#include <stddef.h>
#include "yog/gc/mark-sweep-compact.h"

void* 
YogMarkSweepCompact_alloc(YogMarkSweepCompact* msc, size_t size) 
{
    return NULL;
}

void 
YogMarkSweepCompact_initialize(YogMarkSweepCompact* msc, size_t chunk_size, size_t threshold) 
{
    msc->chunk_size = chunk_size;
    msc->chunks = NULL;
    msc->all_chunks = NULL;
    msc->all_chunks_last = NULL;
    unsigned int i;
    for (i = 0; i < MARK_SWEEP_COMPACT_NUM_SIZE; i++) {
        msc->pages[i] = NULL;
    }
    msc->large_obj = NULL;
    unsigned int sizes[] = { /* 8, 16, 32,*/ 64, 128, 256, 512, 1024, 2048, };
    unsigned int index = 0;
    unsigned int size;
    for (size = 0; size < MARK_SWEEP_COMPACT_SIZE2INDEX_SIZE; size++) {
        if (sizes[index] < size) {
            index++;
        }
        msc->size2index[size] = index;
    }
    for (i = 0; i < MARK_SWEEP_COMPACT_NUM_SIZE; i++) {
        msc->freelist_size[i] = sizes[i];
    }
    msc->header = NULL;
    msc->threshold = threshold;
    msc->allocated_size = 0;
}

void 
YogMarkSweepCompact_finalize(YogMarkSweepCompact* msc) 
{
}

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
