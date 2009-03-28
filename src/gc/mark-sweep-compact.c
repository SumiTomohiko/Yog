#include <inttypes.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include "yog/gc/mark-sweep-compact.h"
#ifdef TEST_MARK_SWEEP_COMPACT
#   include <stdio.h>
#   include <CUnit/Basic.h>
#   include <CUnit/CUnit.h>
#endif

/* TODO: commonize with yog/yog.h */
#define BOOL    int
#define FALSE   (0)
#define TRUE    (!FALSE)

#define SURVIVE_INDEX_MAX    8

#define PAGE_SIZE       4096

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

struct Compactor {
    void (*callback)(struct YogMarkSweepCompact*, struct Compactor*, struct YogMarkSweepCompactHeader*); 
    struct YogMarkSweepCompactChunk* cur_chunk;
    struct YogMarkSweepCompactPage* next_page;
    struct YogMarkSweepCompactPage* cur_page[MARK_SWEEP_COMPACT_NUM_SIZE];
    unsigned int cur_index[MARK_SWEEP_COMPACT_NUM_SIZE];
};

typedef struct Compactor Compactor;

static YogMarkSweepCompactPage* 
ptr2page(void* p) 
{
    return ((YogMarkSweepCompactPage*)((uintptr_t)(p) & ~(PAGE_SIZE - 1)));
}

static void* 
keep_object(YogEnv* env, void* ptr) 
{
    if (ptr == NULL) {
        return NULL;
    }

    YogMarkSweepCompactHeader* header = (YogMarkSweepCompactHeader*)ptr - 1;
    if (!header->marked) {
#if 0
        GcObjectStat_increment_survive_num(&header->stat);
        increment_living_object_number(ENV_VM(env), header->stat.survive_num);
        increment_total_object_number(ENV_VM(env));
#endif
        header->marked = TRUE;

        ChildrenKeeper keeper = header->keeper;
        if (keeper != NULL) {
            (*keeper)(env, ptr, keep_object);
        }
    }

    return ptr;
}

static void 
finalize(YogEnv* env, YogMarkSweepCompactHeader* header) 
{
    if (header->finalizer != NULL) {
        (*header->finalizer)(env, header + 1);
    }
}

static void 
destroy_memory(void* ptr, size_t size) 
{
    memset(ptr, 0xfd, size);
}

static void 
delete(YogMarkSweepCompact* msc, YogMarkSweepCompactHeader* header) 
{
    size_t size = header->size;
    destroy_memory(header, size);

    YogMarkSweepCompactPage* page = ptr2page(header);
    page->num_obj_avail++;
    unsigned int index = msc->size2index[size];
    if (page->num_obj_avail == page->num_obj) {
        YogMarkSweepCompactPage* pages = msc->pages[index];
        if (pages == page) {
            msc->pages[index] = pages->next;
        }
        else {
            while (pages != NULL) {
                if (pages->next == page) {
                    pages->next = page->next;
                    break;
                }
                pages = pages->next;
            }
        }

        YogMarkSweepCompactChunk* chunk = page->chunk;
        ((YogMarkSweepCompactFreeList*)page)->next = chunk->pages;
        chunk->pages = (YogMarkSweepCompactFreeList*)page;
    }
    else {
        if (page->num_obj_avail == 1) {
            page->next = msc->pages[index];
            msc->pages[index] = page;
        }
        ((YogMarkSweepCompactFreeList*)header)->next = page->freelist;
        page->freelist = (YogMarkSweepCompactFreeList*)header;
    }
}

static void 
Compactor_initialize(Compactor* compactor) 
{
    compactor->callback = NULL;
    compactor->cur_chunk = NULL;
    compactor->next_page = NULL;
    unsigned int i;
    for (i = 0; i < MARK_SWEEP_COMPACT_NUM_SIZE; i++) {
        compactor->cur_page[i] = NULL;
        compactor->cur_page[i] = 0;
    }
}

static unsigned int 
object_number_of_page(size_t obj_size) 
{
    return (PAGE_SIZE - sizeof(YogMarkSweepCompactPage)) / obj_size;
}

static void 
iterate_objects(YogMarkSweepCompact* msc, Compactor* compactor) 
{
    YogMarkSweepCompactChunk* chunk = msc->all_chunks;
    while (chunk != NULL) {
        YogMarkSweepCompactPage* page = (YogMarkSweepCompactPage*)chunk->first_page;
        void* chunk_end = (unsigned char*)page + msc->chunk_size;
        while ((void*)page < chunk_end) {
            if (IS_PAGE_USED(page)) {
                unsigned char* obj = (unsigned char*)(page + 1);
                size_t obj_size = page->obj_size;
                unsigned int num_obj = object_number_of_page(obj_size);
                unsigned int i;
                for (i = 0; i < num_obj; i++) {
                    YogMarkSweepCompactHeader* header = (YogMarkSweepCompactHeader*)obj;
                    if (IS_OBJ_USED(header)) {
                        (*compactor->callback)(msc, compactor, header);
                    }

                    obj = obj + obj_size;
                }
            }

            page = (YogMarkSweepCompactPage*)((unsigned char*)page + PAGE_SIZE);
        }

        chunk = chunk->all_chunks_next;
    }
}

static void 
free_chunk(YogMarkSweepCompact* msc, YogMarkSweepCompactChunk* chunk) 
{
    munmap(chunk->first_page, msc->chunk_size);
    free(chunk);
}

static void
free_chunks(YogMarkSweepCompact* msc, Compactor* compactor) 
{
    if (compactor->cur_chunk == NULL) {
        return;
    }

    YogMarkSweepCompactChunk* chunk = compactor->cur_chunk->all_chunks_next;
    while (chunk != NULL) {
        YogMarkSweepCompactChunk* next_chunk = chunk->all_chunks_next;
        free_chunk(msc, chunk);
        chunk = next_chunk;
    }
    compactor->cur_chunk->all_chunks_next = NULL;
}

static void 
remake_freelist(YogMarkSweepCompact* msc, Compactor* compactor, YogMarkSweepCompactPage* first_free_page) 
{
    unsigned int i;
    for (i = 0; i < MARK_SWEEP_COMPACT_NUM_SIZE; i++) {
        YogMarkSweepCompactPage* page = compactor->cur_page[i];
        if ((page != NULL) && (0 < page->num_obj_avail)) {
            unsigned int num_obj_used = page->num_obj - page->num_obj_avail;
            size_t obj_size = msc->freelist_size[i];
            unsigned char* obj = (unsigned char*)page + sizeof(YogMarkSweepCompactPage) + num_obj_used * obj_size;
            page->freelist = (YogMarkSweepCompactFreeList*)obj;
            unsigned char* page_end = (unsigned char*)page + PAGE_SIZE;
            while (obj + obj_size < page_end) {
                YogMarkSweepCompactFreeList* freelist = (YogMarkSweepCompactFreeList*)obj;
                unsigned char* next_obj = obj + obj_size;
                freelist->next = (YogMarkSweepCompactFreeList*)next_obj;

                obj = next_obj;
            }
            ((YogMarkSweepCompactFreeList*)obj)->next = NULL;

            msc->pages[i] = page;
        }
        else {
            msc->pages[i] = NULL;
        }
    }

    if (first_free_page != NULL) {
        YogMarkSweepCompactChunk* chunk = compactor->cur_chunk;
        if (chunk == NULL) {
            return;
        }

        unsigned char* page = (unsigned char*)first_free_page;
        unsigned char* chunk_begin = (unsigned char*)chunk->first_page;
        unsigned char* chunk_end = chunk_begin + msc->chunk_size;
        while (page + PAGE_SIZE < chunk_end) {
            unsigned char* next_page = page + PAGE_SIZE;
            ((YogMarkSweepCompactFreeList*)page)->next = (YogMarkSweepCompactFreeList*)next_page;
            page = next_page;
        }
        ((YogMarkSweepCompactFreeList*)page)->next = NULL;
        chunk->pages = (YogMarkSweepCompactFreeList*)first_free_page;
    }
}

static void 
set_next_page(YogMarkSweepCompact* msc, Compactor* compactor, void* last_page) 
{
    unsigned char* page_end = (unsigned char*)last_page + PAGE_SIZE;
    YogMarkSweepCompactPage* next_page = (YogMarkSweepCompactPage*)page_end;
    size_t chunk_size = msc->chunk_size;
    YogMarkSweepCompactPage* chunk_end = (YogMarkSweepCompactPage*)((unsigned char*)compactor->cur_chunk->first_page + chunk_size);
    if (chunk_end <= next_page) {
        compactor->cur_chunk = compactor->cur_chunk->next;
        if (compactor->cur_chunk != NULL) {
            next_page = compactor->cur_chunk->first_page;
        }
        else {
            next_page = NULL;
        }
    }
    compactor->next_page = next_page;
}

static void 
set_forward_address(YogMarkSweepCompact* msc, Compactor* compactor, YogMarkSweepCompactHeader* header) 
{
    size_t size = header->size;
    unsigned int index = msc->size2index[size];
    size_t rounded_size = msc->freelist_size[index];
    YogMarkSweepCompactPage* page = compactor->cur_page[index];
    unsigned int obj_index;
    if (page == NULL) {
        page = compactor->next_page;

        if (page == NULL) {
            YogMarkSweepCompactChunk* cur_chunk = compactor->cur_chunk;
            if (cur_chunk == NULL) {
                cur_chunk = msc->all_chunks;
                compactor->cur_chunk = cur_chunk;
            }

            page = cur_chunk->first_page;
        }
        set_next_page(msc, compactor, page);

        compactor->cur_page[index] = page;
        compactor->cur_index[index] = obj_index = 0;
    }
    else {
        obj_index = compactor->cur_index[index] + 1;

        unsigned int max_obj_index = object_number_of_page(rounded_size) - 1;
        if (max_obj_index < obj_index) {
            compactor->cur_page[index] = page = compactor->next_page;

            set_next_page(msc, compactor, page);

            obj_index = 0;
        }

        compactor->cur_index[index] = obj_index;
    }

    unsigned int header_size = sizeof(YogMarkSweepCompactPage);
    void* to = (unsigned char*)page + header_size + rounded_size * obj_index;
    header->forwarding_addr = to;
}

static void 
copy_object(YogMarkSweepCompact* msc, Compactor* compactor, YogMarkSweepCompactHeader* header)
{
    size_t size = header->size;
    memcpy(header->forwarding_addr, header, size);

    YogMarkSweepCompactPage* page = ptr2page(header->forwarding_addr);
    unsigned int index = msc->size2index[size];
    if (page != compactor->cur_page[index]) {
        YogMarkSweepCompactChunk* chunk = compactor->cur_chunk;
        if (chunk == NULL) {
            compactor->cur_chunk = chunk = msc->all_chunks;
        }
        else {
            YogMarkSweepCompactPage* chunk_begin = chunk->first_page;
            YogMarkSweepCompactPage* chunk_end = (YogMarkSweepCompactPage*)((unsigned char*)chunk_begin + msc->chunk_size);
            if ((page < chunk_begin) || (chunk_end <= page)) {
                compactor->cur_chunk = chunk = chunk->all_chunks_next;
            }
        }

        page->flags = PAGE_USED;
        page->next = NULL;
        size_t rounded_size = msc->freelist_size[index];
        page->obj_size = rounded_size;
        page->num_obj = page->num_obj_avail = object_number_of_page(rounded_size);
        page->freelist = NULL;
        page->chunk = chunk;

        compactor->cur_page[index] = page;
    }

    page->num_obj_avail--;
}

static void* 
update_pointer(YogEnv* env, void* ptr) 
{
    if (ptr == NULL) {
        return NULL;
    }

    YogMarkSweepCompactHeader* header = (YogMarkSweepCompactHeader*)ptr - 1;
    if (!header->updated) {
        header->updated = TRUE;

        ChildrenKeeper keeper = header->keeper;
        if (keeper != NULL) {
            (*keeper)(env, ptr, update_pointer);
        }

        if (header->prev != NULL) {
            header->prev = header->prev->forwarding_addr;
        }
        if (header->next != NULL) {
            header->next = header->next->forwarding_addr;
        }
    }

    void* to = header->forwarding_addr + 1;
    return to;
}

static void 
compact(YogEnv* env, YogMarkSweepCompact* msc) 
{
    Compactor compactor;
    Compactor_initialize(&compactor);
    compactor.callback = set_forward_address;
    iterate_objects(msc, &compactor);
    YogMarkSweepCompactPage* first_free_page = compactor.next_page;

    (*msc->root_keeper)(env, msc->root, update_pointer);
    YogMarkSweepCompactHeader** front = &msc->header;
    if (*front != NULL) {
        *front = (*front)->forwarding_addr;
    }

    Compactor_initialize(&compactor);
    compactor.callback = copy_object;
    iterate_objects(msc, &compactor);

    free_chunks(msc, &compactor);
    msc->all_chunks_last = compactor.cur_chunk;

    remake_freelist(msc, &compactor, first_free_page);
}

void 
YogMarkSweepCompact_gc(YogEnv* env, YogMarkSweepCompact* msc) 
{
    YogMarkSweepCompactHeader* header = msc->header;
    while (header != NULL) {
        header->updated = header->marked = FALSE;
        header = header->next;
    }

    (*msc->root_keeper)(env, msc->root, keep_object);

    header = msc->header;
    while (header != NULL) {
        YogMarkSweepCompactHeader* next = header->next;

        if (!header->marked) {
            finalize(env, header);

            if (header->prev != NULL) {
                header->prev->next = next;
            }
            else {
                msc->header = next;
            }
            if (next != NULL) {
                next->prev = header->prev;
            }

            delete(msc, header);
        }

        header = next;
    }

    /**
     * TODO: free large objects.
     */

    compact(env, msc);

    msc->allocated_size = 0;
}

static void 
initialize_memory(void* ptr, size_t size) 
{
    memset(ptr, 0xcb, size);
}

void* 
YogMarkSweepCompact_alloc(YogEnv* env, YogMarkSweepCompact* msc, ChildrenKeeper keeper, Finalizer finalizer, size_t size) 
{
    size_t total_size = size + sizeof(YogMarkSweepCompactHeader);
    if (msc->threshold <= msc->allocated_size) {
        YogMarkSweepCompact_gc(env, msc);
    }

    msc->allocated_size += total_size;

    if (total_size < MARK_SWEEP_COMPACT_SIZE2INDEX_SIZE - 1) {
        unsigned int index = msc->size2index[total_size];
        YogMarkSweepCompactPage* page = msc->pages[index];
        if (page == NULL) {
            YogMarkSweepCompactChunk* chunk = msc->chunks;
            if (chunk == NULL) {
                chunk = malloc(sizeof(YogMarkSweepCompactChunk));
                if (chunk == NULL) {
                    msc->err = ERR_MSC_MALLOC;
                    return NULL;
                }

                size_t chunk_size = msc->chunk_size;
                size_t mmap_size = chunk_size + PAGE_SIZE;
                int proto = PROT_READ | PROT_WRITE;
                int flags = MAP_PRIVATE | MAP_ANONYMOUS;
                unsigned char* mmap_begin = mmap(NULL, mmap_size, proto, flags, -1, 0);
                if (mmap_begin == MAP_FAILED) {
                    msc->err = ERR_MSC_MMAP;
                    return NULL;
                }
                unsigned char* chunk_begin = (unsigned char*)(((uintptr_t)mmap_begin + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1));
                if (mmap_begin != chunk_begin) {
                    int retval = munmap(mmap_begin, chunk_begin - mmap_begin);
                    if (retval != 0) {
                        msc->err = ERR_MSC_MUNMAP;
                        return NULL;
                    }
                }
                unsigned char* mmap_end = mmap_begin + mmap_size;
                unsigned char* chunk_end = chunk_begin + chunk_size;
                if (mmap_end != chunk_end) {
                    int retval = munmap(chunk_end, mmap_end - chunk_end);
                    if (retval != 0) {
                        msc->err = ERR_MSC_MUNMAP;
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
YogMarkSweepCompact_initialize(YogEnv* env, YogMarkSweepCompact* msc, size_t chunk_size, size_t threshold, void* root, ChildrenKeeper root_keeper) 
{
    msc->err = ERR_MSC_NONE;
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
    msc->root = root;
    msc->root_keeper = root_keeper;
}

void 
YogMarkSweepCompact_finalize(YogEnv* env, YogMarkSweepCompact* msc) 
{
    /* TODO */
}

#ifdef TEST_MARK_SWEEP_COMPACT
#define CHUNK_SIZE  (1 * 1024 * 1024)
#define THRESHOLD   CHUNK_SIZE

#define CREATE_TEST(name, root, keeper) \
    static void \
    name() \
    { \
        YogMarkSweepCompact msc; \
        YogMarkSweepCompact_initialize(NULL, &msc, CHUNK_SIZE, THRESHOLD, root, keeper); \
        \
        test_##name(&msc); \
        \
        YogMarkSweepCompact_finalize(NULL, &msc); \
    }

static void 
test_alloc1(YogMarkSweepCompact* msc) 
{
    void* ptr = YogMarkSweepCompact_alloc(NULL, msc, NULL, NULL, 0);
    CU_ASSERT_PTR_NOT_NULL(ptr);
}

CREATE_TEST(alloc1, NULL, NULL);

static void 
test_assign_page1(YogMarkSweepCompact* msc) 
{
    YogMarkSweepCompact_alloc(NULL, msc, NULL, NULL, 0);
    CU_ASSERT_PTR_NOT_NULL(msc->pages[0]);
}

CREATE_TEST(assign_page1, NULL, NULL);

static void 
test_use_up_page1(YogMarkSweepCompact* msc) 
{
    size_t size = msc->freelist_size[0];
    unsigned int obj_num = object_number_of_page(size);
    unsigned int i;
    for (i = 0; i < obj_num; i++) {
        YogMarkSweepCompact_alloc(NULL, msc, NULL, NULL, 0);
    }
    CU_ASSERT_PTR_NULL(msc->pages[0]);
}

CREATE_TEST(use_up_page1, NULL, NULL);

static void 
dummy_keeper(YogEnv* env, void* ptr, ObjectKeeper keeper) 
{
}

static void 
test_gc1(YogMarkSweepCompact* msc) 
{
    YogMarkSweepCompact_alloc(NULL, msc, NULL, NULL, 0);
    YogMarkSweepCompact_gc(NULL, msc);
    CU_ASSERT_PTR_NULL(msc->pages[0]);
}

CREATE_TEST(gc1, NULL, dummy_keeper);

static void* gc2_ptr = NULL;

static void 
gc2_keeper(YogEnv* env, void* ptr, ObjectKeeper keeper) 
{
    gc2_ptr = (*keeper)(env, gc2_ptr);
}

static void 
test_gc2(YogMarkSweepCompact* msc) 
{
    gc2_ptr = YogMarkSweepCompact_alloc(NULL, msc, NULL, NULL, 0);
    void* gc2_ptr_old = gc2_ptr;
    YogMarkSweepCompact_gc(NULL, msc);
    CU_ASSERT_PTR_EQUAL(gc2_ptr, gc2_ptr_old);
}

CREATE_TEST(gc2, NULL, gc2_keeper);

static void* gc3_ptr1 = NULL;
static void* gc3_ptr2 = NULL;

static void 
gc3_keeper(YogEnv* env, void* ptr, ObjectKeeper keeper) 
{
    gc3_ptr2 = (*keeper)(env, gc3_ptr2);
}

static void 
test_gc3(YogMarkSweepCompact* msc) 
{
    gc3_ptr1 = YogMarkSweepCompact_alloc(NULL, msc, NULL, NULL, 0);
    gc3_ptr2 = YogMarkSweepCompact_alloc(NULL, msc, NULL, NULL, 0);
    YogMarkSweepCompact_gc(NULL, msc);
    CU_ASSERT_PTR_EQUAL(gc3_ptr2, gc3_ptr1);
}

CREATE_TEST(gc3, NULL, gc3_keeper);

static void 
test_page_freelist1(YogMarkSweepCompact* msc) 
{
    void* ptr1 = YogMarkSweepCompact_alloc(NULL, msc, NULL, NULL, 0);
    YogMarkSweepCompact_gc(NULL, msc);
    void* ptr2 = YogMarkSweepCompact_alloc(NULL, msc, NULL, NULL, 0);
    CU_ASSERT_PTR_EQUAL(ptr1, ptr2);
}

CREATE_TEST(page_freelist1, NULL, dummy_keeper);

static void 
test_page_freelist2(YogMarkSweepCompact* msc) 
{
    size_t size = msc->freelist_size[0];
    unsigned int obj_num = object_number_of_page(size);
    unsigned int i;
    for (i = 0; i < obj_num; i++) {
        YogMarkSweepCompact_alloc(NULL, msc, NULL, NULL, 0);
    }
    YogMarkSweepCompact_gc(NULL, msc);

    unsigned int page_num = msc->chunk_size / PAGE_SIZE;
    unsigned char* page = (unsigned char*)msc->chunks->first_page;
    for (i = 0; i < page_num - 1; i++) {
        unsigned char* page_begin = page + PAGE_SIZE * i;
        unsigned char* page_end = page_begin + PAGE_SIZE;
        CU_ASSERT_PTR_EQUAL(*(void**)page_begin, page_end);
    }
    unsigned char* page_begin = page + PAGE_SIZE * (page_num - 1);
    CU_ASSERT_PTR_NULL(*(void**)page_begin);
}

CREATE_TEST(page_freelist2, NULL, dummy_keeper);

static void 
test_page_freelist3(YogMarkSweepCompact* msc) 
{
    size_t size = msc->freelist_size[0];
    unsigned int obj_num = object_number_of_page(size);
    unsigned int i;
    for (i = 0; i < obj_num; i++) {
        YogMarkSweepCompact_alloc(NULL, msc, NULL, NULL, 0);
    }
    YogMarkSweepCompact_gc(NULL, msc);

    unsigned int page_num = msc->chunk_size / PAGE_SIZE;
    unsigned char* page = (unsigned char*)msc->chunks->first_page;
    for (i = 0; i < page_num - 1; i++) {
        unsigned char* page_begin = page + PAGE_SIZE * i;
        unsigned char* page_end = page_begin + PAGE_SIZE;
        CU_ASSERT_PTR_EQUAL(*(void**)page_begin, page_end);
    }
    unsigned char* page_begin = page + PAGE_SIZE * (page_num - 1);
    CU_ASSERT_PTR_NULL(*(void**)page_begin);
}

CREATE_TEST(page_freelist3, NULL, dummy_keeper);

static void* page_freelist4_ptr = NULL;

static void 
page_freelist4_keep_children(YogEnv* env, void* ptr, ObjectKeeper keeper) 
{
    page_freelist4_ptr = (*keeper)(env, page_freelist4_ptr);
}

static void 
test_page_freelist4(YogMarkSweepCompact* msc) 
{
    size_t size = msc->freelist_size[0];
    unsigned int obj_num = object_number_of_page(size) + 1;
    unsigned int i;
    for (i = 1; i < obj_num; i++) {
        YogMarkSweepCompact_alloc(NULL, msc, NULL, NULL, 0);
    }
    page_freelist4_ptr = YogMarkSweepCompact_alloc(NULL, msc, NULL, NULL, 0);

    YogMarkSweepCompact_gc(NULL, msc);

    unsigned int page_num = msc->chunk_size / PAGE_SIZE - 1;
    unsigned char* page = (unsigned char*)msc->chunks->first_page;
    for (i = 0; i < page_num - 1; i++) {
        unsigned char* page_begin = page + PAGE_SIZE * (i + 1);
        unsigned char* page_end = page_begin + PAGE_SIZE;
        CU_ASSERT_PTR_EQUAL(*(void**)page_begin, page_end);
    }
    unsigned char* page_begin = page + PAGE_SIZE * page_num;
    CU_ASSERT_PTR_NULL(*(void**)page_begin);
}

CREATE_TEST(page_freelist4, NULL, page_freelist4_keep_children);

static void* page_freelist5_ptr = NULL;

static void 
page_freelist5_keep_children(YogEnv* env, void* ptr, ObjectKeeper keeper) 
{
    page_freelist5_ptr = (*keeper)(env, page_freelist5_ptr);
}

static void 
test_page_freelist5(YogMarkSweepCompact* msc) 
{
    size_t size = msc->freelist_size[0];
    unsigned int obj_num = object_number_of_page(size) + 1;
    unsigned int i;
    for (i = 1; i < obj_num; i++) {
        YogMarkSweepCompact_alloc(NULL, msc, NULL, NULL, 0);
    }
    page_freelist5_ptr = YogMarkSweepCompact_alloc(NULL, msc, NULL, NULL, 0);

    YogMarkSweepCompact_gc(NULL, msc);

    unsigned char* first_page = (unsigned char*)msc->chunks->first_page;
    unsigned char* page = first_page + PAGE_SIZE;
    CU_ASSERT_PTR_EQUAL(msc->chunks->pages, page);
}

CREATE_TEST(page_freelist5, NULL, page_freelist5_keep_children);

static void 
test_page1(YogMarkSweepCompact* msc) 
{
    unsigned int index = 0;
    size_t size = msc->freelist_size[index];
    unsigned int obj_num = object_number_of_page(size) + 1;
    unsigned int i;
    for (i = 0; i < obj_num - 1; i++) {
        YogMarkSweepCompact_alloc(NULL, msc, NULL, NULL, 0);
    }
    void* p = YogMarkSweepCompact_alloc(NULL, msc, NULL, NULL, 0);
    void* page = ptr2page(p);
    CU_ASSERT_PTR_EQUAL(msc->pages[index], page);
}

CREATE_TEST(page1, NULL, NULL);

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

    CU_pSuite suite = CU_add_suite("mark-sweep-compact", NULL, NULL);
    if (suite == NULL) {
        ERROR("failed CU_add_suite");
    }

#define ADD_TEST(name)  do { \
    CU_pTest test = CU_add_test(suite, #name, name); \
    if (test == NULL) { \
        ERROR("failed CU_add_test %s", #name); \
    } \
} while (0)
    ADD_TEST(alloc1);
    ADD_TEST(assign_page1);
    ADD_TEST(gc1);
    ADD_TEST(gc2);
    ADD_TEST(gc3);
    ADD_TEST(page1);
    ADD_TEST(page_freelist1);
    ADD_TEST(page_freelist2);
    ADD_TEST(page_freelist3);
    ADD_TEST(page_freelist4);
    ADD_TEST(page_freelist5);
    ADD_TEST(use_up_page1);
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
