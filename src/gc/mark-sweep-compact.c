#include <inttypes.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include "yog/gc/mark-sweep-compact.h"

/* TODO: commonize with yog/yog.h */
#define BOOL    int
#define FALSE   (0)
#define TRUE    (!FALSE)

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

static void 
gc(YogMarkSweepCompact* msc) 
{
}

unsigned int 
object_number_of_page(size_t obj_size) 
{
    return (PAGE_SIZE - sizeof(YogMarkSweepCompactPage)) / obj_size;
}

static void 
initialize_memory(void* ptr, size_t size) 
{
    memset(ptr, 0xcb, size);
}

void* 
YogMarkSweepCompact_alloc(YogMarkSweepCompact* msc, ChildrenKeeper keeper, Finalizer finalizer, size_t size) 
{
    size_t total_size = size + sizeof(YogMarkSweepCompactHeader);
    if (msc->threshold <= msc->allocated_size) {
        gc(msc);
    }

    msc->allocated_size += total_size;

    if (total_size < MARK_SWEEP_COMPACT_SIZE2INDEX_SIZE - 1) {
        unsigned int index = msc->size2index[total_size];
        YogMarkSweepCompactPage* page = msc->pages[index];
        if (page == NULL) {
            YogMarkSweepCompactChunk* chunk = msc->chunks;
            if (chunk == NULL) {
                chunk = malloc(sizeof(YogMarkSweepCompactChunk));

                size_t chunk_size = msc->chunk_size;
                size_t mmap_size = chunk_size + PAGE_SIZE;
                int proto = PROT_READ | PROT_WRITE;
                int flags = MAP_PRIVATE | MAP_ANONYMOUS;
                unsigned char* mmap_begin = mmap(NULL, mmap_size, proto, flags, -1, 0);
                if (mmap_begin == MAP_FAILED) {
                    msc->err = ERR_MMAP;
                    return NULL;
                }
                unsigned char* chunk_begin = (unsigned char*)(((uintptr_t)mmap_begin + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1));
                if (mmap_begin != chunk_begin) {
                    int retval = munmap(mmap_begin, chunk_begin - mmap_begin);
                    if (retval != 0) {
                        msc->err = ERR_MUNMAP;
                        return NULL;
                    }
                }
                unsigned char* mmap_end = mmap_begin + mmap_size;
                unsigned char* chunk_end = chunk_begin + chunk_size;
                if (mmap_end != chunk_end) {
                    int retval = munmap(chunk_end, mmap_end - chunk_end);
                    if (retval != 0) {
                        msc->err = ERR_MUNMAP;
                        return NULL;
                    }
                }

                YogMarkSweepCompactFreeList* pages = (YogMarkSweepCompactFreeList*)mmap_begin;
                unsigned int num_pages = chunk_size / PAGE_SIZE;
                unsigned int i;
                for (i = 0; i < num_pages - 1; i++) {
                    pages->next = (YogMarkSweepCompactFreeList*)((unsigned char*)pages + PAGE_SIZE);
                    pages = pages->next;
                }
                pages->next = NULL;

                chunk->pages = (YogMarkSweepCompactFreeList*)mmap_begin;
                chunk->first_page = (YogMarkSweepCompactPage*)mmap_begin;

                chunk->next = NULL;
                msc->chunks = chunk;
                if (msc->all_chunks != NULL) {
                    msc->all_chunks_last->all_chunks_next = chunk;
                }
                else {
                    msc->all_chunks = chunk;
                }
                msc->all_chunks_last = chunk;
            }

            page = (YogMarkSweepCompactPage*)chunk->pages;
            chunk->pages = chunk->pages->next;
            if (chunk->pages == NULL) {
                msc->chunks = chunk->next;
            }
            msc->pages[index] = page;

            unsigned int size = msc->freelist_size[index];
            unsigned int num_obj = object_number_of_page(size);
            YogMarkSweepCompactFreeList* obj = (YogMarkSweepCompactFreeList*)(page + 1);
            page->freelist = obj;
            unsigned int i;
            for (i = 0; i < num_obj - 1; i++) {
                obj->next = (YogMarkSweepCompactFreeList*)((unsigned char*)obj + size);
                obj = obj->next;
            }
            obj->next = NULL;

            page->flags = PAGE_USED;
            page->next = NULL;
            page->obj_size = size;
            page->num_obj = num_obj;
            page->num_obj_avail = num_obj;
            page->chunk = chunk;
        }

        YogMarkSweepCompactHeader* header = (YogMarkSweepCompactHeader*)page->freelist;
        page->freelist = page->freelist->next;
        page->num_obj_avail--;
        if (page->num_obj_avail == 0) {
            msc->pages[index] = page->next;
        }

        initialize_memory(header, total_size);
#if 0
        GcObjectStat_initialize(&header->stat);
#endif
        header->prev = NULL;
        header->next = msc->header;
        if (msc->header != NULL) {
            msc->header->prev = header;
        }
        msc->header = header;

        header->flags = OBJ_USED;
        header->forwarding_addr = NULL;
        header->size = total_size;
        header->keeper = keeper;
        header->finalizer = finalizer;
        header->updated = header->marked = FALSE;

        return header + 1;
    }
    else {
        /* TODO */
        abort();
    }

    return NULL;
}

void 
YogMarkSweepCompact_initialize(YogMarkSweepCompact* msc, size_t chunk_size, size_t threshold) 
{
    msc->err = ERR_NONE;
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
