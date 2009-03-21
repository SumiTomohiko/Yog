#include <stddef.h>
#include "yog/gc/mark-sweep-compact.h"

/* TODO: commonize with yog/yog.h */
#define BOOL    int
#define FALSE   (0)
#define TRUE    (!FALSE)
typedef struct YogEnv YogEnv;
typedef void* (*ObjectKeeper)(YogEnv*, void*);
typedef void (*ChildrenKeeper)(YogEnv*, void*, ObjectKeeper);
typedef void (*Finalizer)(YogEnv*, void*);

#define SURVIVE_INDEX_MAX    8

#define PAGE_SIZE       4096
#define PTR2PAGE(p)     ((YogMarkSweepCompactPage*)((uintptr_t)(p) & ~(PAGE_SIZE - 1)))

struct YogMarkSweepCompactFreeList {
    struct YogMarkSweepCompactFreeList* next;
};

typedef struct YogMarkSweepCompactFreeList YogMarkSweepCompactFreeList;

struct YogMarkSweepCompactChunk {
    struct YogMarkSweepCompactChunk* next;
    struct YogMarkSweepCompactChunk* all_chunks_next;
    struct YogMarkSweepCompactFreeList* pages;
    struct YogMarkSweepCompactPage* first_page;
};

typedef struct YogMarkSweepCompactChunk YogMarkSweepCompactChunk;

struct YogMarkSweepCompactPage {
    unsigned int flags;
    struct YogMarkSweepCompactPage* next;
    size_t obj_size;
    unsigned int num_obj;
    unsigned int num_obj_avail;
    struct YogMarkSweepCompactFreeList* freelist;
    struct YogMarkSweepCompactChunk* chunk;
};

#define PAGE_USED           0x01
#define IS_PAGE_USED(p)     ((p)->flags & PAGE_USED)

typedef struct YogMarkSweepCompactPage YogMarkSweepCompactPage;

struct YogMarkSweepCompactHeader {
    unsigned int flags;
#if 0
    struct GcObjectStat stat;
#endif
    struct YogMarkSweepCompactHeader* forwarding_addr;
    struct YogMarkSweepCompactHeader* prev;
    struct YogMarkSweepCompactHeader* next;
    ChildrenKeeper keeper;
    Finalizer finalizer;
    BOOL marked;
    BOOL updated;
    size_t size;
};

#define OBJ_USED        0x01
#define IS_OBJ_USED(o)  ((o)->flags & OBJ_USED)

typedef struct YogMarkSweepCompactHeader YogMarkSweepCompactHeader;

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
