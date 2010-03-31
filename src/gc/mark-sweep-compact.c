#include "yog/config.h"
#include <inttypes.h>
#include <signal.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include "yog/error.h"
#include "yog/gc.h"
#include "yog/gc/mark-sweep-compact.h"
#if defined(GC_GENERATIONAL)
#   include "yog/gc/generational.h"
#endif
#include "yog/sysdeps.h"
#include "yog/vm.h"
#include "yog/yog.h"

#define BITS_PER_BYTE   8

#define SURVIVE_INDEX_MAX    8

#define PAGE_SIZE       4096

#define IS_SMALL_OBJ(size)  ((size) < MARK_SWEEP_COMPACT_SIZE2INDEX_SIZE - 1)
#define ADDR2HEADER(addr)   ((YogMarkSweepCompactHeader*)(addr) - 1)

struct YogMarkSweepCompactFreeList {
    struct YogMarkSweepCompactFreeList* next;
};

typedef struct YogMarkSweepCompactFreeList YogMarkSweepCompactFreeList;

struct YogMarkSweepCompactChunk {
    struct YogMarkSweepCompactChunk* next;
    struct YogMarkSweepCompactChunk* all_chunks_next;
    struct YogMarkSweepCompactFreeList* pages;
    struct YogMarkSweepCompactPage* first_page;
#if defined(GC_GENERATIONAL)
    unsigned char* grey_page_flags;
#endif
};

typedef struct YogMarkSweepCompactChunk YogMarkSweepCompactChunk;

typedef void (*Callback)(struct YogMarkSweepCompact*, struct YogCompactor*, struct YogMarkSweepCompactHeader*);

typedef void (*IteratePagesCallback)(YogEnv*, YogMarkSweepCompact*, YogMarkSweepCompactChunk*, uint_t, YogMarkSweepCompactPage*);
typedef BOOL (*PageSelector)(YogMarkSweepCompactChunk*, uint_t, YogMarkSweepCompactPage*);

void
YogMarkSweepCompact_prepare(YogEnv* env, YogMarkSweepCompact* msc)
{
    DEBUG(TRACE("%p: unmark all", env));
    YogMarkSweepCompactHeader* header = msc->header;
    while (header != NULL) {
        DEBUG(TRACE("%p: header=%p", env, header));
        header->updated = header->marked = FALSE;
        header = header->next;
    }
}

static YogMarkSweepCompactPage*
ptr2page(void* p)
{
    return ((YogMarkSweepCompactPage*)((uintptr_t)(p) & ~(PAGE_SIZE - 1)));
}

void*
YogMarkSweepCompact_mark_recursively(YogEnv* env, void* ptr, ObjectKeeper obj_keeper, void* heap)
{
    if (ptr == NULL) {
        return NULL;
    }

    YogMarkSweepCompactHeader* header = (YogMarkSweepCompactHeader*)ptr - 1;
    if (!header->marked) {
        DEBUG(TRACE("%p: mark: %p", env, ptr));
        header->marked = TRUE;

        ChildrenKeeper keeper = header->keeper;
        if (keeper != NULL) {
            DEBUG(TRACE("%p: keeper=%p", env, keeper));
            (*keeper)(env, ptr, obj_keeper, heap);
        }
    }

    return ptr;
}

static void
finalize(YogEnv* env, YogMarkSweepCompactHeader* header)
{
    DEBUG(TRACE("%p: finalize: %p", env, header));
    if (header->finalizer != NULL) {
        (*header->finalizer)(env, header + 1);
    }
}

static void
destroy_memory(void* ptr, size_t size)
{
    memset(ptr, 0xfd, size);
}

#if defined(GC_GENERATIONAL)
static void
get_index_of_page_table(YogMarkSweepCompactPage* page, uint_t* flag_index, uint_t* bit_index)
{
    YogMarkSweepCompactChunk* chunk = page->chunk;
    YogMarkSweepCompactPage* first_page = chunk->first_page;
    uint_t page_offset = (uintptr_t)page - (uintptr_t)first_page;
    uint_t page_index = page_offset / PAGE_SIZE;
    *flag_index = page_index / BITS_PER_BYTE;
    *bit_index = page_index % BITS_PER_BYTE;
}

static void
white_page(void* ptr)
{
    YogMarkSweepCompactPage* page = ptr2page(ptr);
    DEBUG(TRACE("white page: %p-%p", page, (unsigned char*)page + PAGE_SIZE));
    uint_t flag_index = 0;
    uint_t bit_index = 0;
    get_index_of_page_table(page, &flag_index, &bit_index);
    page->chunk->grey_page_flags[flag_index] &= ~(1 << bit_index);
}

void
YogMarkSweepCompact_grey_page(void* ptr)
{
    YogMarkSweepCompactPage* page = ptr2page(ptr);
    DEBUG(TRACE("grey page: %p-%p", page, (unsigned char*)page + PAGE_SIZE));
    uint_t flag_index = 0;
    uint_t bit_index = 0;
    get_index_of_page_table(page, &flag_index, &bit_index);
    page->chunk->grey_page_flags[flag_index] |= 1 << bit_index;
}
#endif

static void
delete(YogMarkSweepCompact* msc, YogMarkSweepCompactHeader* header)
{
    size_t size = header->size;
    DEBUG(TRACE("delete: %p", header + 1));
    destroy_memory(header, size);

    if (IS_SMALL_OBJ(size)) {
        YogMarkSweepCompactPage* page = ptr2page(header);
        page->num_obj_avail++;
        uint_t index = msc->size2index[size];
        if (page->num_obj_avail == page->num_obj) {
            DEBUG(TRACE("page %p is empty", page));
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

#if defined(GC_GENERATIONAL)
            white_page(page);
#endif
        }
        else {
            DEBUG(TRACE("page %p is not empty", page));
            if (page->num_obj_avail == 1) {
                page->next = msc->pages[index];
                msc->pages[index] = page;
            }
            ((YogMarkSweepCompactFreeList*)header)->next = page->freelist;
            DEBUG(TRACE("*(void**)header(%p)=%p", header, *(void**)header));
            page->freelist = (YogMarkSweepCompactFreeList*)header;
        }
    }
    else {
        munmap(header, size);
    }
}

void
YogCompactor_init(YogEnv* env, YogCompactor* compactor)
{
    compactor->cur_chunk = NULL;
    compactor->next_page = NULL;
    uint_t i;
    for (i = 0; i < MARK_SWEEP_COMPACT_NUM_SIZE; i++) {
        compactor->cur_page[i] = NULL;
        compactor->cur_page[i] = 0;
    }
}

static uint_t
object_number_of_page(size_t obj_size)
{
    return (PAGE_SIZE - sizeof(YogMarkSweepCompactPage)) / obj_size;
}

static void
iterate_objects(YogMarkSweepCompact* msc, YogCompactor* compactor, Callback callback)
{
    YogMarkSweepCompactChunk* chunk = msc->all_chunks;
    while (chunk != NULL) {
        YogMarkSweepCompactPage* page = (YogMarkSweepCompactPage*)chunk->first_page;
        void* chunk_end = (unsigned char*)page + msc->chunk_size;
        while ((void*)page < chunk_end) {
            if (IS_PAGE_USED(page)) {
                unsigned char* obj = (unsigned char*)(page + 1);
                size_t obj_size = page->obj_size;
                uint_t num_obj = object_number_of_page(obj_size);
                uint_t i;
                for (i = 0; i < num_obj; i++) {
                    YogMarkSweepCompactHeader* header = (YogMarkSweepCompactHeader*)obj;
                    if (IS_OBJ_USED(header)) {
                        (*callback)(msc, compactor, header);
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
#if defined(GC_GENERATIONAL)
    free(chunk->grey_page_flags);
#endif
    free(chunk);
}

static void
free_chunks(YogMarkSweepCompact* msc, YogCompactor* compactor)
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
remake_freelist(YogMarkSweepCompact* msc, YogCompactor* compactor, YogMarkSweepCompactPage* first_free_page)
{
    uint_t i;
    for (i = 0; i < MARK_SWEEP_COMPACT_NUM_SIZE; i++) {
        YogMarkSweepCompactPage* page = compactor->cur_page[i];
        if ((page != NULL) && (0 < page->num_obj_avail)) {
            uint_t num_obj_used = page->num_obj - page->num_obj_avail;
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
set_next_page(YogMarkSweepCompact* msc, YogCompactor* compactor, void* last_page)
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
set_forward_address(YogMarkSweepCompact* msc, YogCompactor* compactor, YogMarkSweepCompactHeader* header)
{
    size_t size = header->size;
    uint_t index = msc->size2index[size];
    size_t rounded_size = msc->freelist_size[index];
    YogMarkSweepCompactPage* page = compactor->cur_page[index];
    uint_t obj_index;
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

        uint_t max_obj_index = object_number_of_page(rounded_size) - 1;
        if (max_obj_index < obj_index) {
            compactor->cur_page[index] = page = compactor->next_page;

            set_next_page(msc, compactor, page);

            obj_index = 0;
        }

        compactor->cur_index[index] = obj_index;
    }

    size_t page_header_size = sizeof(YogMarkSweepCompactPage);
    size_t obj_header_size = sizeof(YogMarkSweepCompactHeader);
    void* to = (char*)page + page_header_size + rounded_size * obj_index + obj_header_size;
    header->forwarding_addr = to;
}

static void
move_obj(YogMarkSweepCompact* msc, YogCompactor* compactor, YogMarkSweepCompactHeader* header)
{
    size_t size = header->size;
    YogMarkSweepCompactHeader* dest_header = (YogMarkSweepCompactHeader*)header->forwarding_addr - 1;
    DEBUG(TRACE("move: %p->%p", header, dest_header));
    memcpy(dest_header, header, size);

    YogMarkSweepCompactPage* page = ptr2page(header->forwarding_addr);
    uint_t index = msc->size2index[size];
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

void*
YogMarkSweepCompact_update_ptr(YogEnv* env, void* ptr, void* heap)
{
    if (ptr == NULL) {
        return NULL;
    }

    YogMarkSweepCompactHeader* header = (YogMarkSweepCompactHeader*)ptr - 1;
    if (header->updated) {
        return header->forwarding_addr;
    }
    header->updated = TRUE;

    ChildrenKeeper children_keeper = header->keeper;
    if (children_keeper != NULL) {
        (*children_keeper)(env, ptr, YogMarkSweepCompact_update_ptr, heap);
    }

    if (header->prev != NULL) {
        header->prev = ADDR2HEADER(header->prev->forwarding_addr);
    }
    if (header->next != NULL) {
        header->next = ADDR2HEADER(header->next->forwarding_addr);
    }

    return header->forwarding_addr;
}

void
YogMarkSweepCompact_alloc_virtually(YogEnv* env, YogMarkSweepCompact* msc, YogCompactor* compactor)
{
    iterate_objects(msc, compactor, set_forward_address);
}

void
YogMarkSweepCompact_move_objs(YogEnv* env, YogMarkSweepCompact* msc, YogCompactor* compactor)
{
    iterate_objects(msc, compactor, move_obj);
}

void
YogMarkSweepCompact_update_front_header(YogEnv* env, YogMarkSweepCompact* msc)
{
    YogMarkSweepCompactHeader** front = &msc->header;
    if (*front == NULL) {
        return;
    }
    *front = ADDR2HEADER((*front)->forwarding_addr);
}

void
YogMarkSweepCompact_shrink(YogEnv* env, YogMarkSweepCompact* msc, YogCompactor* compactor, YogMarkSweepCompactPage* first_free_page)
{
    free_chunks(msc, compactor);
    msc->all_chunks_last = compactor->cur_chunk;
    remake_freelist(msc, compactor, first_free_page);
}

void
YogMarkSweepCompact_delete_garbage(YogEnv* env, YogMarkSweepCompact* msc)
{
    YogMarkSweepCompactHeader* header = msc->header;
    while (header != NULL) {
        YogMarkSweepCompactHeader* next = header->next;
        DEBUG(TRACE("%p: header=%p", env, header));

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
}

#if defined(GC_MARK_SWEEP_COMPACT)
static void*
keep_object(YogEnv* env, void* ptr, void* heap)
{
    return YogMarkSweepCompact_mark_recursively(env, ptr, keep_object, heap);
}

void
YogMarkSweepCompact_keep_vm(YogEnv* env, YogMarkSweepCompact* msc)
{
    YogVM_keep_children(env, env->vm, keep_object, msc);
}

void
YogMarkSweepCompact_post_gc(YogEnv* env, YogMarkSweepCompact* msc)
{
    msc->allocated_size = 0;
}
#endif

static void
init_memory(void* ptr, size_t size)
{
    memset(ptr, 0xcb, size);
}

#if defined(GC_GENERATIONAL)

static int_t
protect_page(void* page, int_t prot)
{
    DEBUG(TRACE("protect page: page=%p-%p, PROT_READ=%d, PROT_WRITE=%d", page, (char*)page + PAGE_SIZE, prot & PROT_READ ? 1 : 0, prot & PROT_WRITE ? 1 : 0));
    return mprotect(page, PAGE_SIZE, prot);
}
#endif

void*
YogMarkSweepCompact_alloc(YogEnv* env, YogMarkSweepCompact* msc, ChildrenKeeper keeper, Finalizer finalizer, size_t size)
{
    size_t total_size = size + sizeof(YogMarkSweepCompactHeader);
    if (env->vm->gc_stress || (msc->threshold <= msc->allocated_size)) {
#if defined(GC_MARK_SWEEP_COMPACT)
        YogGC_perform(env);
#elif defined(GC_GENERATIONAL)
        if (!msc->in_gc) {
            YogGC_perform_major(env);
        }
#endif
    }

    msc->allocated_size += total_size;

    YogMarkSweepCompactHeader* header;
#define ERROR(reason)   do { \
    msc->err = reason; \
    return NULL; \
} while (0)
#if 0
    BOOL mmapped = FALSE;
#endif
    if (IS_SMALL_OBJ(total_size)) {
        uint_t index = msc->size2index[total_size];
        YogMarkSweepCompactPage* page = msc->pages[index];
        if (page == NULL) {
            YogMarkSweepCompactChunk* chunk = msc->chunks;
            if (chunk == NULL) {
                chunk = malloc(sizeof(YogMarkSweepCompactChunk));
                if (chunk == NULL) {
                    ERROR(ERR_MSC_MALLOC);
                }

                size_t chunk_size = msc->chunk_size;
                size_t mmap_size = chunk_size + PAGE_SIZE;
                int_t proto = PROT_READ | PROT_WRITE;
#if !defined(MAP_ANONYMOUS)
#   if defined(MAP_ANON)
#       define MAP_ANONYMOUS MAP_ANON
#   endif
#endif
                int_t flags = MAP_PRIVATE | MAP_ANONYMOUS;
                unsigned char* mmap_begin = mmap(NULL, mmap_size, proto, flags, -1, 0);
                if (mmap_begin == MAP_FAILED) {
                    ERROR(ERR_MSC_MMAP);
                }
                unsigned char* chunk_begin = (unsigned char*)(((uintptr_t)mmap_begin + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1));
                if (mmap_begin != chunk_begin) {
                    int_t retval = munmap(mmap_begin, chunk_begin - mmap_begin);
                    if (retval != 0) {
                        ERROR(ERR_MSC_MUNMAP);
                    }
                }
                unsigned char* mmap_end = mmap_begin + mmap_size;
                unsigned char* chunk_end = chunk_begin + chunk_size;
                if (mmap_end != chunk_end) {
                    int_t retval = munmap(chunk_end, mmap_end - chunk_end);
                    if (retval != 0) {
                        ERROR(ERR_MSC_MUNMAP);
                    }
                }
                DEBUG(TRACE("%p: heap: %p-%p", env, chunk_begin, chunk_end));

                YogMarkSweepCompactFreeList* pages = (YogMarkSweepCompactFreeList*)mmap_begin;
                uint_t num_pages = chunk_size / PAGE_SIZE;
                uint_t i;
                for (i = 0; i < num_pages - 1; i++) {
                    pages->next = (YogMarkSweepCompactFreeList*)((unsigned char*)pages + PAGE_SIZE);
                    pages = pages->next;
                }
                pages->next = NULL;

                chunk->pages = (YogMarkSweepCompactFreeList*)mmap_begin;
                chunk->first_page = (YogMarkSweepCompactPage*)mmap_begin;

#if defined(GC_GENERATIONAL)
                size_t flags_size = (num_pages + BITS_PER_BYTE - 1) & ~(BITS_PER_BYTE - 1);
                chunk->grey_page_flags = malloc(flags_size);
                YogSysdeps_bzero(chunk->grey_page_flags, flags_size);
#endif

                chunk->next = chunk->all_chunks_next = NULL;
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
            DEBUG(TRACE("%p: assign page: %p", env, page));

            uint_t size = msc->freelist_size[index];
            uint_t num_obj = object_number_of_page(size);
            YogMarkSweepCompactFreeList* obj = (YogMarkSweepCompactFreeList*)(page + 1);
            page->freelist = obj;
            uint_t i;
            for (i = 0; i < num_obj - 1; i++) {
                obj->next = (YogMarkSweepCompactFreeList*)((unsigned char*)obj + size);
                DEBUG(TRACE("%p: *(void**)(obj=%p)=%p", env, obj, *(void**)obj));
                obj = obj->next;
            }
            obj->next = NULL;
            DEBUG(TRACE("%p: *(void**)(obj=%p)=%p", env, obj, *(void**)obj));

            page->flags = PAGE_USED;
            page->next = NULL;
            page->obj_size = size;
            page->num_obj = num_obj;
            page->num_obj_avail = num_obj;
            page->chunk = chunk;

#if defined(GC_GENERATIONAL)
            if (!msc->in_gc) {
                if (protect_page(page, PROT_READ) != 0) {
                    ERROR(ERR_MSC_MPROTECT);
                }
            }
#endif
        }

        header = (YogMarkSweepCompactHeader*)page->freelist;
        page->freelist = page->freelist->next;
        page->num_obj_avail--;
        if (page->num_obj_avail == 0) {
            msc->pages[index] = page->next;
        }
    }
    else {
#if 0
        int_t prot = PROT_READ | PROT_WRITE;
        int_t flags = MAP_PRIVATE | MAP_ANONYMOUS;
        void* ptr = mmap(NULL, total_size, prot, flags, -1, 0);
        if (ptr == MAP_FAILED) {
            ERROR(ERR_MSC_MMAP);
        }
        header = ptr;
        mmapped = TRUE;
#endif
        YOG_BUG(env, "large object of size %u is not supported", total_size);
    }
#undef ERROR

    init_memory(header, total_size);

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
#if defined(GC_GENERATIONAL)
    header->gen = GEN_OLD;

#if 0
    if (msc->in_gc && !mmapped) {
#endif
    if (msc->in_gc) {
        /* XXX: only when running major GC? */
        YogMarkSweepCompact_grey_page(header);
    }
#endif

    return header + 1;
}

YogHeap*
YogMarkSweepCompact_new(YogEnv* env, size_t chunk_size, size_t threshold)
{
    YogMarkSweepCompact* msc = (YogMarkSweepCompact*)YogGC_malloc(env, sizeof(YogMarkSweepCompact));
    YogHeap_init(env, (YogHeap*)msc);

    msc->err = ERR_MSC_NONE;
    msc->chunk_size = chunk_size;
    msc->chunks = NULL;
    msc->all_chunks = NULL;
    msc->all_chunks_last = NULL;
    uint_t i;
    for (i = 0; i < MARK_SWEEP_COMPACT_NUM_SIZE; i++) {
        msc->pages[i] = NULL;
    }
    uint_t sizes[] = { /* 8, 16, 32,*/ 64, 128, 256, 512, 1024, 2048, };
    uint_t index = 0;
    uint_t size;
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
#if defined(GC_GENERATIONAL)
    msc->in_gc = FALSE;
    msc->grey_pages = NULL;
#endif
}

void
YogMarkSweepCompact_delete(YogEnv* env, YogHeap* heap)
{
    YogMarkSweepCompact* msc = (YogHeap*)heap;
    YogMarkSweepCompactHeader* header = msc->header;
    while (header != NULL) {
        YogMarkSweepCompactHeader* next = header->next;

        finalize(env, header);
        delete(msc, header);

        header = next;
    }

    YogMarkSweepCompactChunk* chunk = msc->all_chunks;
    while (chunk != NULL) {
        YogMarkSweepCompactChunk* next = chunk->all_chunks_next;
        free_chunk(msc, chunk);
        chunk = next;
    }
}

BOOL
YogMarkSweepCompact_is_empty(YogEnv* env, YogMarkSweepCompact* msc)
{
    if (msc->header == NULL) {
        return TRUE;
    }
    else {
        return FALSE;
    }
}

#if defined(GC_GENERATIONAL)
static void*
minor_gc_keep_object(YogEnv* env, void* ptr, void* heap)
{
    if (ptr == NULL) {
        return NULL;
    }
    if (!IS_YOUNG(ptr)) {
        DEBUG(TRACE("%p: %p is in old generation.", env, ptr));
        return ptr;
    }

    env->vm->has_young_ref = TRUE;

    return YogGenerational_copy_young_object(env, ptr, minor_gc_keep_object, heap);
}

#define ITERATE_PAGES(env, msc, test_page, proc, heap)  do { \
    YogMarkSweepCompactChunk* chunk = msc->all_chunks; \
    while (chunk != NULL) { \
        uint_t page_num = msc->chunk_size / PAGE_SIZE; \
        uint_t i; \
        for (i = 0; i < page_num; i++) { \
            unsigned char* first_page = (unsigned char*)chunk->first_page; \
            unsigned char* page_begin = first_page + PAGE_SIZE * i; \
            YogMarkSweepCompactPage* page = (YogMarkSweepCompactPage*)page_begin; \
            if (test_page(chunk, i, page)) { \
                proc(env, msc, chunk, i, page, heap); \
            } \
        } \
\
        chunk = chunk->all_chunks_next; \
    } \
} while (0)

#define FLAGS(chunk, page_index) \
    (chunk)->grey_page_flags[(page_index) / BITS_PER_BYTE]
#define BIT_POS(page_index) ((page_index) % BITS_PER_BYTE)
#define IS_GREY_PAGE(chunk, page_index, page) \
    ((FLAGS((chunk), (page_index)) & (1 << BIT_POS((page_index)))) != 0)

#define TRACE_GREY_OBJECTS(env, msc, chunk, page_index, page, heap)     do { \
    size_t obj_size = page->obj_size; \
    uint_t obj_num = object_number_of_page(obj_size); \
    uint_t j; \
    env->vm->has_young_ref = FALSE; \
    for (j = 0; j < obj_num; j++) { \
        unsigned char* obj = (unsigned char*)page + sizeof(YogMarkSweepCompactPage) + obj_size * j; \
        YogMarkSweepCompactHeader* header = (YogMarkSweepCompactHeader*)obj; \
        if (IS_OBJ_USED(header)) { \
            if (header->marked) { \
                DEBUG(TRACE("grey object marked: %p", header + 1)); \
                env->vm->has_young_ref = TRUE; \
            } \
            else { \
                DEBUG(TRACE("grey object kept: %p", header + 1)); \
                ChildrenKeeper keeper = header->keeper; \
                if (keeper != NULL) { \
                    DEBUG(TRACE("grey object keeper of %p: %p", header + 1, keeper)); \
                    (*keeper)(env, header + 1, minor_gc_keep_object, heap); \
                } \
            } \
        } \
    } \
    if (!env->vm->has_young_ref) { \
        white_page(page); \
        protect_page(page, PROT_READ); \
    } \
} while (0)

void
YogMarkSweepCompact_trace_grey_children(YogEnv* env, YogMarkSweepCompact* msc, void* heap)
{
    YogGenerational* generational = heap;
    ITERATE_PAGES(env, msc, IS_GREY_PAGE, TRACE_GREY_OBJECTS, generational);
}

#undef TRACE_GREY_OBJECTS

#define IS_WHITE_PAGE(chunk, page_index, page) \
    (IS_PAGE_USED((page)) && !IS_GREY_PAGE((chunk), (page_index), (page)))
#define PROTECT_WHITE_PAGE(env, msc, chunk, page_index, page, heap)     do { \
    protect_page(page, PROT_READ); \
} while (0)

void
YogMarkSweepCompact_protect_white_pages(YogEnv* env, YogMarkSweepCompact* msc)
{
    ITERATE_PAGES(env, msc, IS_WHITE_PAGE, PROTECT_WHITE_PAGE, msc);
}

#undef PROTECT_WHITE_PAGE
#undef IS_WHITE_PAGE
#undef IS_GREY_PAGE
#undef BIT_POS
#undef FLAGS

static void
sigsegv_handler(int sig, siginfo_t* sip, void* scp)
{
    void* fault_address = sip->si_addr;
    if (fault_address == NULL) {
        return;
    }

    YogMarkSweepCompactPage* page = ptr2page(fault_address);
    DEBUG(TRACE("page=%p", page));
    if (protect_page(page, PROT_READ | PROT_WRITE) != 0) {
        YOG_BUG(NULL, "protect_page failed");
    }

    YogMarkSweepCompact_grey_page(fault_address);
}

BOOL
YogMarkSweepCompact_install_sigsegv_handler(YogEnv* env)
{
    struct sigaction action;
    action.sa_sigaction = sigsegv_handler;
    sigemptyset(&action.sa_mask);
    action.sa_flags = SA_SIGINFO;
    if (sigaction(SIGSEGV, &action, NULL) != 0) {
        YOG_BUG(env, "sigaction failed");
    }

    return TRUE;
}

void
YogMarkSweepCompact_unprotect_all_pages(YogEnv* env, YogMarkSweepCompact* msc)
{
    YogMarkSweepCompactChunk* chunk = msc->all_chunks;
    while (chunk != NULL) {
        int_t prot = PROT_READ | PROT_WRITE;
        if (mprotect(chunk->first_page, msc->chunk_size, prot) != 0) {
            YOG_BUG(env, "mprotect failed");
        }
        chunk = chunk->all_chunks_next;
    }
}
#endif

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
