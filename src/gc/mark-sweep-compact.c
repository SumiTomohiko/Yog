#include "yog/config.h"
#include <errno.h>
#include <stddef.h>
#include <sys/mman.h>
#include "yog/error.h"
#include "yog/gc.h"
#include "yog/gc/mark-sweep-compact.h"
#include "yog/misc.h"
#include "yog/vm.h"
#include "yog/yog.h"

struct Header {
    struct Header* prev;
    struct Header* next;
    ChildrenKeeper keeper;
    Finalizer finalizer;
    BOOL marked;
};

typedef struct Header Header;

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

struct MarkSweepCompact {
    struct YogHeap base;
    struct ChunkHeader* arena;
    uint_t arena_size;
    struct FreeHeader* small[SMALL_NUM];
    struct FreeHeader* large;
    struct Header* header;
};

typedef struct MarkSweepCompact MarkSweepCompact;

void
YogMarkSweepCompact_delete(YogEnv* env, YogHeap* heap)
{
    YogMarkSweepCompact_delete_garbage(env, heap);

    MarkSweepCompact* msc = (MarkSweepCompact*)heap;
    if (munmap(msc->arena, msc->arena_size) != 0) {
        YogError_raise_sys_err(env, errno, YNIL);
    }

    YogGC_free(env, heap, sizeof(MarkSweepCompact));
}

static FreeHeader*
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
    return (FreeHeader*)ptr;
}

static BOOL
find_large_chunk(YogEnv* env, MarkSweepCompact* msc, size_t size, FreeHeader** pchunk, FreeHeader*** plist)
{
    FreeHeader* header = msc->large;
    if (header == NULL) {
        return FALSE;
    }

    FreeHeader* best_header = NULL;
    uint_t best_size = UINT_MAX;
    while (header != NULL) {
        uint_t chunk_size = CHUNK_SIZE(header);
        if ((size <= chunk_size) && (chunk_size < best_size)) {
            best_header = header;
            best_size = chunk_size;
        }
        header = header->next;
    }
    if (best_header == NULL) {
        return FALSE;
    }
    *pchunk = best_header;
    *plist = &msc->large;
    return TRUE;
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
gc(YogEnv* env)
{
#if defined(GC_MARK_SWEEP_COMPACT)
    YogGC_perform(env);
#else
    YogGC_perform_major(env);
#endif
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
        ADD_TO_LIST(msc->large, chunk);
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

static void*
alloc(YogEnv* env, MarkSweepCompact* msc, size_t size)
{
    if (env->vm->gc_stress) {
        gc(env);
    }

    size_t size_including_header = size + sizeof(ChunkHeader);
    FreeHeader* chunk;
    FreeHeader** list;
#define FIND_BEST_CHUNK do { \
    if (find_best_chunk(env, msc, size_including_header, &chunk, &list)) { \
        return handle_free_chunk(env, msc, size_including_header, chunk, list); \
    } \
} while (0)
    FIND_BEST_CHUNK;
    gc(env);
    FIND_BEST_CHUNK;
    YogGC_compact(env);
    FIND_BEST_CHUNK;
    return NULL;
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
        return &msc->large;
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

static void*
keep_object(YogEnv* env, void* ptr, void* heap)
{
    if (ptr == NULL) {
        return NULL;
    }
    Header* header = (Header*)ptr - 1;
    if (header->marked) {
        return ptr;
    }

    header->marked = TRUE;

    ChildrenKeeper keeper = header->keeper;
    if (keeper == NULL) {
        return ptr;
    }
    (*keeper)(env, ptr, keep_object, heap);

    return ptr;
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

    FreeHeader* chunk = mmap_anonymous(env, size);
    uint_t arena_size = size - sizeof(ChunkHeader);
    FreeHeader_init(env, chunk, arena_size, TRUE);
    FreeFooter* footer = chunk2footer(chunk);
    footer->size = size;
    ChunkHeader* sentinel = NEXT_CHUNK(chunk);
    CHUNK_PREV_USED(sentinel) = FALSE; /* unused */
    CHUNK_SIZE(sentinel) = 0; /* unused */
    CHUNK_USED(sentinel) = TRUE;

    heap->arena = CHUNK(chunk);
    heap->arena_size = size;

    uint_t i;
    for (i = 0; i < SMALL_NUM; i++) {
        heap->small[i] = NULL;
    }
    heap->large = chunk;

    return (YogHeap*)heap;
}

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

int
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
