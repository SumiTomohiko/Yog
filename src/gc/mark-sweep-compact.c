#include "yog/config.h"
#include <errno.h>
#include <stddef.h>
/* <sys/types.h> must be before <sys/mman.h> */
#include <sys/types.h>
#include <sys/mman.h>
#include "yog/error.h"
#include "yog/gc.h"
#if defined(GC_GENERATIONAL)
#   include "yog/gc/internal.h"
#endif
#include "yog/gc/mark-sweep-compact.h"
#include "yog/misc.h"
#include "yog/vm.h"
#include "yog/yog.h"

/**
 * Basic idea of this allocator is same as that of dlmalloc.
 *
 * = Glossary
 *
 * == Arena
 *
 * One large memory area. An arena is splitted into plural chunks.
 *
 * Arenas construct a linked list.
 *
 * == Chunk
 *
 * One memory area returned to a client application (Yog) for each allocation.
 * There are two kinds of chunks -- used chunk and free chunk. A used chunk
 * consists of a header and a payload. A free chunk consists of a header, a
 * footer and unused payload. The structure ChunkHeader represents used chunks'
 * headers. The structure FreeHeader represents free chunks' headers. The
 * structure FreeFooter represents free chunks' footer.
 *
 * == Payload
 *
 * One memory area which a client application (Yog) can use.
 *
 * = Memory Layout
 *
 * == Arena
 *
 * +----------------------------+ head
 * |   pointer to next arena    |
 * +----------------------------+
 * |                            |
 * : used chunks / free chunks  :
 * |                            |
 * +----------------------------+
 * |    used chunk (sentinel)   |
 * +----------------------------+ tail
 *
 * A used chunk without a payload is at the end of an arena. This is a sentinel.
 *
 * == Used Chunk
 *
 * +----------------------------+
 * |         prev_used          | ^
 * +----------------------------+ |
 * |           size             | | header
 * +----------------------------+ |
 * |         used (TRUE)        | v
 * +----------------------------+
 * |                            |
 * :         payload            :
 * |                            |
 * +----------------------------+
 *
 * prev_used becomes TRUE when a PREVIOUS chunk is used. size is a size of this
 * chunk.
 *
 * == Free Chunk
 *
 * +----------------------------+
 * |         prev_used          | ^
 * +----------------------------+ |
 * |           size             | |
 * +----------------------------+ |
 * |         used (FALSE)       | | header
 * +----------------------------+ |
 * |           prev             | |         ^
 * +----------------------------+ |         |
 * |           next             | v         |
 * +----------------------------+           |
 * |                            |           | become payload
 * :        unused area         :           |
 * |                            |           |
 * +----------------------------+           |
 * |           size             | <- footer v
 * +----------------------------+
 *
 * First three members (prev_used, size and used) are same as these of a used
 * chunk. size member in a footer is same as size member in a header. When a
 * free chunk becomes a used chunk, prev and next members in a header and size
 * member in footer become parts of payload.
 *
 * = Links
 *
 * == Small
 *
 * +---+---+---+-----+---+
 * | 24| 32| 40| ... |512| small[SMALL_NUM]
 * +---+---+---+-----+---+
 *   |
 *   v
 * +---+
 * |   | A free chunk of size 24
 * +---+
 *   ^
 *   |
 *   v
 * +---+
 * |   |
 * +---+
 *
 * There are SMALL_NUM links. Each link concatenates chunks of same size. In
 * allocating memory, the allocator finds the best chunk from this links. A
 * found chunk is removed from a link and separated into two chunks. The first
 * chunk is returned to Yog, the second chunk is added to a link.
 *
 * = Large
 *
 * +---+
 * |   | large
 * +---+
 *   |
 *   v
 * +---+
 * |   | A free chunk of any size
 * +---+
 *   ^
 *   |
 *   v
 * +---+
 * |   |
 * +---+
 *
 * Chunks which size is greater than MAX_SMALL_SIZE are concatenated to one
 * link.
 *
 * = Deallocating
 *
 * 1.
 * +----------------------------+
 * |          free              |
 * +----------------------------+
 * |  chunk to be deallocated   |
 * +----------------------------+
 * |          free              |
 * +----------------------------+
 *
 * 2.
 * +----------------------------+
 * |                            |
 * |        free (merged)       |
 * |                            |
 * +----------------------------+
 * |          free              |
 * +----------------------------+
 *
 * 3.
 * +----------------------------+
 * |                            |
 * |                            |
 * |        free (merged)       |
 * |                            |
 * |                            |
 * +----------------------------+
 *
 * When used chank is collected by GC, this chunk and previous/next free chunks
 * are merged into one free chunk. This larger free chunk is added to a
 * small/large link.
 */

struct Header {
    struct Header* prev;
    struct Header* next;
    ChildrenKeeper keeper;
    Finalizer finalizer;
    BOOL marked;
#if defined(GC_GENERATIONAL)
    struct OldHeader generational_part;
#endif
};

typedef struct Header Header;

#define PAYLOAD2HEADER(ptr) ((Header*)(ptr) - 1)

struct ChunkHeader {
    BOOL prev_used;
    uint_t size;
    BOOL used;
};

typedef struct ChunkHeader ChunkHeader;

#define CHUNK(chunk)            ((ChunkHeader*)(chunk))
#define CHUNK_PREV_USED(chunk)  CHUNK((chunk))->prev_used
#define CHUNK_SIZE(chunk)       CHUNK((chunk))->size
#define CHUNK_USED(chunk)       CHUNK((chunk))->used
#define NEXT_CHUNK(chunk)       CHUNK((char*)(chunk) + CHUNK_SIZE((chunk)))
#define payload2chunk(p)        CHUNK((char*)(p) - sizeof(ChunkHeader))

struct FreeHeader {
    struct ChunkHeader base;
    struct FreeHeader* prev;
    struct FreeHeader* next;
};

typedef struct FreeHeader FreeHeader;

#define chunk2footer(chunk) \
    ((FreeFooter*)((char*)(chunk) + CHUNK_SIZE((chunk))) - 1)
#define FREE(chunk)         ((FreeHeader*)(chunk))
#define FREE_PREV(chunk)    FREE((chunk))->prev
#define FREE_NEXT(chunk)    FREE((chunk))->next

struct FreeFooter {
    uint_t size;
};

typedef struct FreeFooter FreeFooter;

#define ALIGNMENT           8
#define size2index(size)    \
    ((size) < MIN_SMALL_SIZE ? 0 : ((size) - MIN_SMALL_SIZE) / ALIGNMENT)
#define MAX_SMALL_SIZE      512
#define MIN_SMALL_SIZE      (sizeof(FreeHeader) + sizeof(FreeFooter))
#define SMALL_NUM           (size2index(MAX_SMALL_SIZE) + 1)

#define LARGE_NUM       32
#define LARGE_SHIFT     8

struct Arena {
    struct Arena* next;
};

#define ARENA_CHUNKS(arena) ((FreeHeader*)((arena) + 1))

typedef struct Arena Arena;

struct MarkSweepCompact {
    struct YogHeap base;
    struct Arena* arenas;
    uint_t arena_size;
    struct FreeHeader* small[SMALL_NUM];
    struct FreeHeader* large[LARGE_NUM];
    struct Header* header;
    size_t allocated_size;
};

typedef struct MarkSweepCompact MarkSweepCompact;

static void
delete_arena(YogEnv* env, Arena* arena, size_t size)
{
    if (munmap(arena, size) != 0) {
        YogError_raise_sys_err(env, errno, YNIL);
    }
}

void
YogMarkSweepCompact_delete(YogEnv* env, YogHeap* heap)
{
    YogMarkSweepCompact_delete_garbage(env, heap);

    MarkSweepCompact* msc = (MarkSweepCompact*)heap;
    Arena* arena = msc->arenas;
    while (arena != NULL) {
        Arena* next = arena->next;
        delete_arena(env, arena, msc->arena_size);
        arena = next;
    }

    YogGC_free(env, heap, sizeof(MarkSweepCompact));
}

static Arena*
mmap_anonymous(YogEnv* env, size_t size)
{
#if !defined(MAP_ANONYMOUS) && defined(MAP_ANON)
#   define MAP_ANONYMOUS MAP_ANON
#endif
    int prot = PROT_READ | PROT_WRITE;
    int flags = MAP_PRIVATE | MAP_ANONYMOUS;
    void* ptr = mmap(NULL, size, prot, flags, -1, 0);
    if (ptr == MAP_FAILED) {
        YogError_raise_sys_err(env, errno, YNIL);
    }
    return (Arena*)ptr;
}

static uint_t
compute_large_index(YogEnv* env, size_t size)
{
    uint_t x = size >> LARGE_SHIFT;
    if (x == 0) {
        return 0;
    }
    if (1 << (LARGE_NUM >> 1) <= x) {
        return LARGE_NUM - 1;
    }
    uint_t k;
    __asm__("bsrl\t%1, %0\n\t" : "=r" (k) : "g" (x));
    return (k << 1) + ((size >> (k + LARGE_SHIFT - 1)) & 1);
}

static BOOL
find_large_chunk(YogEnv* env, MarkSweepCompact* msc, size_t size, FreeHeader** pchunk, FreeHeader*** plist)
{
    uint_t index = compute_large_index(env, size);
    uint_t i;
    for (i = index; i < LARGE_NUM; i++) {
        FreeHeader* best_header = NULL;
        uint_t best_size = UINT_MAX;
        FreeHeader* header = msc->large[i];
        while (header != NULL) {
            uint_t chunk_size = CHUNK_SIZE(header);
            if (size == chunk_size) {
                best_header = header;
                best_size = chunk_size;
                break;
            }
            if ((size < chunk_size) && (chunk_size < best_size)) {
                best_header = header;
                best_size = chunk_size;
            }
            header = header->next;
        }
        if (best_header != NULL) {
            *pchunk = best_header;
            *plist = &msc->large[i];
            return TRUE;
        }
    }

    return FALSE;
}

static BOOL
find_small_chunk(YogEnv* env, MarkSweepCompact* msc, size_t size, FreeHeader** pchunk, FreeHeader*** plist)
{
    uint_t index = size2index(size);
    uint_t i;
    for (i = index; i < SMALL_NUM; i++) {
        if (msc->small[i] != NULL) {
            break;
        }
    }
    if (i == SMALL_NUM) {
        return find_large_chunk(env, msc, size, pchunk, plist);
    }
    *pchunk = msc->small[i];
    *plist = &msc->small[i];
    return TRUE;
}

static BOOL
find_best_chunk(YogEnv* env, MarkSweepCompact* msc, size_t size, FreeHeader** pchunk, FreeHeader*** plist)
{
    if (MAX_SMALL_SIZE < size) {
        return find_large_chunk(env, msc, size, pchunk, plist);
    }
    return find_small_chunk(env, msc, size, pchunk, plist);
}

static void
ChunkHeader_init(YogEnv* env, ChunkHeader* chunk, size_t size, BOOL prev_used, BOOL used)
{
    CHUNK_PREV_USED(chunk) = prev_used;
    CHUNK_SIZE(chunk) = size;
    CHUNK_USED(chunk) = used;
}

static void
FreeHeader_init(YogEnv* env, FreeHeader* chunk, size_t size, BOOL prev_used)
{
    ChunkHeader_init(env, CHUNK(chunk), size, prev_used, FALSE);
    FREE_PREV(chunk) = FREE_NEXT(chunk) = NULL;
}

static void
init_free_chunk(YogEnv* env, FreeHeader* chunk, size_t size)
{
    FreeHeader_init(env, chunk, size, TRUE);
    FreeFooter* footer = chunk2footer(chunk);
    footer->size = size;
}

static FreeHeader*
split_chunk(YogEnv* env, FreeHeader* chunk, size_t size)
{
    uint_t rest_size = CHUNK_SIZE(chunk) - size;
    if (rest_size < MIN_SMALL_SIZE) {
        return NULL;
    }
    CHUNK_SIZE(chunk) = size;
    FreeHeader* rest = (FreeHeader*)NEXT_CHUNK(chunk);
    init_free_chunk(env, rest, rest_size);
    return rest;
}

static void
add_chunk(YogEnv* env, MarkSweepCompact* msc, FreeHeader* chunk)
{
    if (MAX_SMALL_SIZE < CHUNK_SIZE(chunk)) {
        uint_t index = compute_large_index(env, CHUNK_SIZE(chunk));
        ADD_TO_LIST(msc->large[index], chunk);
        return;
    }
    uint_t index = size2index(CHUNK_SIZE(chunk));
    ADD_TO_LIST(msc->small[index], chunk);
}

static void*
handle_free_chunk(YogEnv* env, MarkSweepCompact* msc, size_t size, FreeHeader* chunk, FreeHeader** list)
{
    DELETE_FROM_LIST(*list, chunk);
    FreeHeader* rest = split_chunk(env, chunk, size);
    if (rest != NULL) {
        add_chunk(env, msc, rest);
    }
    else {
        CHUNK_PREV_USED(NEXT_CHUNK(chunk)) = TRUE;
    }
    CHUNK_USED(chunk) = TRUE;

    return (ChunkHeader*)chunk + 1;
}

#if defined(GC_GENERATIONAL)
static BOOL
check_if_over_threshold(YogEnv* env, MarkSweepCompact* msc, size_t size, size_t threshold)
{
    size_t next_threshold = msc->allocated_size == 0 ? threshold : (msc->allocated_size + threshold - 1) / threshold * threshold;
    return (msc->allocated_size < next_threshold) && (next_threshold <= msc->allocated_size + size);
}

static void
turn_on_major_or_compaction(YogEnv* env, MarkSweepCompact* msc, size_t size)
{
    if (check_if_over_threshold(env, msc, size, msc->arena_size / 3 * 2)) {
        msc->allocated_size = 0;
        env->vm->compaction_flag = TRUE;
        return;
    }
    if (check_if_over_threshold(env, msc, size, msc->arena_size / 2)) {
        env->vm->major_gc_flag = TRUE;
    }
}
#endif

static Arena*
alloc_arena(YogEnv* env, size_t size, Arena* next)
{
    Arena* arena = mmap_anonymous(env, size);
    arena->next = next;

    FreeHeader* chunk = ARENA_CHUNKS(arena);
    size_t sentinel_size = sizeof(ChunkHeader);
    size_t usable_size = size - sentinel_size - sizeof(Arena);
    FreeHeader_init(env, chunk, usable_size, TRUE);
    chunk2footer(chunk)->size = usable_size; /* unused */

    ChunkHeader* sentinel = NEXT_CHUNK(chunk);
    CHUNK_PREV_USED(sentinel) = FALSE; /* unused */
    CHUNK_SIZE(sentinel) = 0; /* unused */
    CHUNK_USED(sentinel) = TRUE;

    return arena;
}

static void
add_arena(YogEnv* env, MarkSweepCompact* msc)
{
    Arena* arena = alloc_arena(env, msc->arena_size, msc->arenas);
    add_chunk(env, msc, ARENA_CHUNKS(arena));
    msc->arenas = arena;
}

static void*
alloc(YogEnv* env, MarkSweepCompact* msc, size_t size)
{
    size_t size_including_header = size + sizeof(ChunkHeader);
    FreeHeader* chunk = NULL; /* -Wall complicates uninitialized using */
    FreeHeader** list = NULL;
#define FIND_BEST_CHUNK do { \
    if (find_best_chunk(env, msc, size_including_header, &chunk, &list)) { \
        return handle_free_chunk(env, msc, size_including_header, chunk, list); \
    } \
} while (0)

#if defined(GC_MARK_SWEEP_COMPACT)
    if (env->vm->gc_stress) {
        YogGC_perform(env);
        YogGC_compact(env);
    }

    FIND_BEST_CHUNK;
    YogGC_perform(env);
    FIND_BEST_CHUNK;
    YogGC_compact(env);
    FIND_BEST_CHUNK;
    add_arena(env, msc);
    FIND_BEST_CHUNK;
    return NULL;
#elif defined(GC_GENERATIONAL)
    turn_on_major_or_compaction(env, msc, size_including_header);
    msc->allocated_size += size_including_header;
    FIND_BEST_CHUNK;
    add_arena(env, msc);
    FIND_BEST_CHUNK;
    return NULL;
#endif
#undef FIND_BEST_CHUNK
}

void*
YogMarkSweepCompact_alloc(YogEnv* env, YogHeap* heap, ChildrenKeeper keeper, Finalizer finalizer, size_t size)
{
    MarkSweepCompact* msc = (MarkSweepCompact*)heap;
    size_t rounded_size = (size + ALIGNMENT - 1) & ~(ALIGNMENT - 1);
    void* ptr = alloc(env, msc, rounded_size + sizeof(Header));
    if (ptr == NULL) {
        return NULL;
    }
    Header* header = (Header*)ptr;
    ADD_TO_LIST(msc->header, header);
    header->keeper = keeper;
    header->finalizer = finalizer;
    header->marked = FALSE;
    void* user_area = header + 1;
    YogGC_init_memory(env, user_area, rounded_size);
    return user_area;
}

static void
finalize(YogEnv* env, Header* header)
{
    if (header->finalizer == NULL) {
        return;
    }
    (*header->finalizer)(env, header + 1);
}

static FreeHeader**
find_list_of_size(YogEnv* env, MarkSweepCompact* msc, size_t size)
{
    if (MAX_SMALL_SIZE < size) {
        return &msc->large[compute_large_index(env, size)];
    }
    return &msc->small[size2index(size)];
}

static FreeHeader*
merge_with_prev_chunk(YogEnv* env, MarkSweepCompact* msc, ChunkHeader* chunk)
{
    FreeFooter* prev_footer = (FreeFooter*)chunk - 1;
    uint_t prev_size = prev_footer->size;
    FreeHeader* prev_chunk = (FreeHeader*)((char*)chunk - prev_size);
    FreeHeader** list = find_list_of_size(env, msc, prev_size);
    DELETE_FROM_LIST(*list, prev_chunk);
    uint_t size = prev_size + CHUNK_SIZE(chunk);
    CHUNK_SIZE(prev_chunk) = size;
    FreeFooter* footer = chunk2footer(prev_chunk);
    footer->size = size;
    return prev_chunk;
}

static FreeHeader*
merge_with_next_chunk(YogEnv* env, MarkSweepCompact* msc, FreeHeader* chunk)
{
    FreeHeader* next_chunk = FREE(NEXT_CHUNK(chunk));
    uint_t next_size = CHUNK_SIZE(next_chunk);
    FreeHeader** list = find_list_of_size(env, msc, next_size);
    DELETE_FROM_LIST(*list, next_chunk);
    uint_t size = CHUNK_SIZE(chunk) + next_size;
    CHUNK_SIZE(chunk) = size;
    FreeFooter* footer = chunk2footer(chunk);
    footer->size = size;
    return chunk;
}

static void
delete(YogEnv* env, MarkSweepCompact* msc, Header* header)
{
    ChunkHeader* chunk = (ChunkHeader*)header - 1;
    FreeHeader* merged_chunk1;
    if (CHUNK_PREV_USED(chunk)) {
        CHUNK_USED(chunk) = FALSE;
        FREE_PREV(chunk) = FREE_NEXT(chunk) = NULL;
        chunk2footer(chunk)->size = CHUNK_SIZE(chunk);
        merged_chunk1 = FREE(chunk);
    }
    else {
        merged_chunk1 = merge_with_prev_chunk(env, msc, chunk);
    }

    ChunkHeader* next_chunk = NEXT_CHUNK(merged_chunk1);
    FreeHeader* merged_chunk2;
    if (CHUNK_USED(next_chunk)) {
        CHUNK_PREV_USED(next_chunk) = FALSE;
        merged_chunk2 = merged_chunk1;
    }
    else {
        merged_chunk2 = merge_with_next_chunk(env, msc, merged_chunk1);
    }

    add_chunk(env, msc, merged_chunk2);
}

void
YogMarkSweepCompact_delete_garbage(YogEnv* env, YogHeap* heap)
{
    MarkSweepCompact* msc = (MarkSweepCompact*)heap;
    Header* header = msc->header;
    while (header != NULL) {
        Header* current = header;
        header = header->next;
        if (current->marked) {
            current->marked = FALSE;
            continue;
        }
        finalize(env, current);
        DELETE_FROM_LIST(msc->header, current);
        delete(env, msc, current);
    }
}

void*
YogMarkSweepCompact_mark_recursively(YogEnv* env, void* ptr, ObjectKeeper keeper, void* heap)
{
    if (ptr == NULL) {
        return NULL;
    }
    Header* header = PAYLOAD2HEADER(ptr);
    if (header->marked) {
        return ptr;
    }

    header->marked = TRUE;

    ChildrenKeeper children_keeper = header->keeper;
    if (children_keeper == NULL) {
        return ptr;
    }
    (*children_keeper)(env, ptr, keeper, heap);

    return ptr;
}

static void*
keep_object(YogEnv* env, void* ptr, void* heap)
{
    return YogMarkSweepCompact_mark_recursively(env, ptr, keep_object, heap);
}

void
YogMarkSweepCompact_keep_root(YogEnv* env, void* ptr, ChildrenKeeper keeper, YogHeap* heap)
{
    (*keeper)(env, ptr, keep_object, heap);
}

BOOL
YogMarkSweepCompact_is_empty(YogEnv* env, YogHeap* heap)
{
    MarkSweepCompact* msc = (MarkSweepCompact*)heap;
    return msc->header == NULL ? TRUE : FALSE;
}

YogHeap*
YogMarkSweepCompact_new(YogEnv* env, size_t size)
{
    MarkSweepCompact* heap = (MarkSweepCompact*)YogGC_malloc(env, sizeof(MarkSweepCompact));
    YogHeap_init(env, (YogHeap*)heap);

    heap->header = NULL;

    Arena* arena = alloc_arena(env, size, NULL);
    heap->arenas = arena;
    heap->arena_size = size;

    uint_t i;
    for (i = 0; i < SMALL_NUM; i++) {
        heap->small[i] = NULL;
    }
    for (i = 0; i < LARGE_NUM; i++) {
        heap->large[i] = NULL;
    }
    add_chunk(env, heap, ARENA_CHUNKS(arena));

    heap->allocated_size = 0;

    return (YogHeap*)heap;
}

#if defined(GC_GENERATIONAL)
ChildrenKeeper
YogMarkSweepCompact_get_children_keeper(YogEnv* env, YogHeap* heap, void* ptr)
{
    return PAYLOAD2HEADER(ptr)->keeper;
}
#endif

#if defined(TEST)
#include <string.h>
#include <CUnit/Basic.h>
#include <CUnit/CUnit.h>

#define TEST_PTR(expected, actual) do { \
    CU_ASSERT((void*)(expected) == (void*)(actual)); \
} while (0)
#define HEAP_SIZE (1024 * 1024)

YogEnv env = ENV_INIT;
YogVM vm;
void* ptr1;

static void
init_test()
{
    ptr1 = NULL;
}

static void
test_chunk_used()
{
    init_test();
    YogHeap* heap = YogMarkSweepCompact_new(&env, HEAP_SIZE);
    void* p = YogMarkSweepCompact_alloc(&env, heap, NULL, NULL, 1);
    ChunkHeader* chunk = payload2chunk((char*)p - sizeof(Header));
    CU_ASSERT(chunk->used);
}

static void
test_alloc1()
{
    init_test();
    YogHeap* heap = YogMarkSweepCompact_new(&env, HEAP_SIZE);
    void* p = YogMarkSweepCompact_alloc(&env, heap, NULL, NULL, 1);
    MarkSweepCompact* msc = (MarkSweepCompact*)heap;
    TEST_PTR(payload2chunk((char*)p - sizeof(Header)), msc->arena);
}

#define TEST_OVERLAP(ptr1, ptr2) do { \
    ChunkHeader* chunk1 = payload2chunk((char*)(ptr1) - sizeof(Header)); \
    ChunkHeader* chunk2 = payload2chunk((char*)(ptr2) - sizeof(Header)); \
    CU_ASSERT((chunk2 < chunk1) || (NEXT_CHUNK(chunk1) <= chunk2)); \
    ChunkHeader* end = NEXT_CHUNK(chunk2); \
    CU_ASSERT((end <= chunk1) || (NEXT_CHUNK(chunk1) < end)); \
} while (0)

static void
test_overlap1()
{
    init_test();
    YogHeap* heap = YogMarkSweepCompact_new(&env, HEAP_SIZE);
    void* p1 = YogMarkSweepCompact_alloc(&env, heap, NULL, NULL, 1);
    void* p2 = YogMarkSweepCompact_alloc(&env, heap, NULL, NULL, 1);
    TEST_OVERLAP(p1, p2);
}

static void
traverse_test_ptrs(YogEnv* env, void* ptr, ObjectKeeper keeper, void* heap)
{
    YogVal v = PTR2VAL(ptr1);
    YogGC_keep(env, &v, keeper, heap);
}

static void
gc_for_test(YogEnv* env, YogHeap* heap)
{
    YogMarkSweepCompact_keep_root(env, NULL, traverse_test_ptrs, heap);
    YogMarkSweepCompact_delete_garbage(env, heap);
}

static void
test_overlap2()
{
    init_test();
    YogHeap* heap = YogMarkSweepCompact_new(&env, HEAP_SIZE);
    YogMarkSweepCompact_alloc(&env, heap, NULL, NULL, 1);
    ptr1 = YogMarkSweepCompact_alloc(&env, heap, NULL, NULL, 1);
    gc_for_test(&env, heap);
    void* p = YogMarkSweepCompact_alloc(&env, heap, NULL, NULL, 1);
    TEST_OVERLAP(ptr1, p);
}

#define ESCAPE_PROTOTYPE
ESCAPE_PROTOTYPE int
main(int argc, const char* argv[])
{
    memset(&vm, 0, sizeof(YogVM));
    env.vm = &vm;

    CU_initialize_registry();
    CU_pSuite suite = CU_add_suite("mark-sweep-compact", NULL, NULL);
    CU_add_test(suite, "test_alloc1", test_alloc1);
    CU_add_test(suite, "test_overlap1", test_overlap1);
    CU_add_test(suite, "test_overlap2", test_overlap2);
    CU_add_test(suite, "test_chunk_used", test_chunk_used);
    CU_basic_run_tests();
    CU_cleanup_registry();

    return 0;
}
#endif

/**
 * vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
 */
