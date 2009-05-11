#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#if defined(TEST_COPYING)
#   include <stdio.h>
#   include <CUnit/Basic.h>
#   include <CUnit/CUnit.h>
#endif
#include "yog/env.h"
#include "yog/gc.h"
#include "yog/gc/copying.h"
#include "yog/thread.h"
#include "yog/vm.h"
#include "yog/yog.h"

#if 0
#   define DEBUG
#endif

#define IS_IN_HEAP(ptr, heap)   do { \
    void* from = heap->items; \
    void* to = heap->items + heap->size; \
    return (from <= ptr) && (ptr < to); \
} while (0)

BOOL 
YogCopying_is_in_active_heap(YogEnv* env, YogCopying* copying, void* ptr) 
{
    YogCopyingHeap* heap = copying->active_heap;
    IS_IN_HEAP(ptr, heap);
}

BOOL 
YogCopying_is_in_inactive_heap(YogEnv* env, YogCopying* copying, void* ptr) 
{
    YogCopyingHeap* heap = copying->inactive_heap;
    IS_IN_HEAP(ptr, heap);
}

#undef IS_IN_HEAP

/* TODO: commonize with the other GC */
static void 
initialize_memory(void* ptr, size_t size) 
{
    memset(ptr, 0xcb, size);
}

static YogCopyingHeap* 
YogCopyingHeap_new(YogCopying* copying, size_t size)
{
    YogCopyingHeap* heap = malloc(sizeof(YogCopyingHeap) + size);
    if (heap == NULL) {
        copying->err = ERR_COPYING_MALLOC;
        return NULL;
    }

    heap->size = size;
    heap->free = heap->items;
    initialize_memory(heap->items, size);

    return heap;
}

static size_t
round_size(size_t size) 
{
    size_t unit = sizeof(void*);
    return (size + unit - 1) & ~(unit - 1);
}

void* 
YogCopying_copy(YogEnv* env, YogCopying* copying, void* ptr) 
{
#if defined(DEBUG)
#   define PRINT(...)   DPRINTF("copy: " __VA_ARGS__)
#else
#   define PRINT(...)
#endif
    if (ptr == NULL) {
#if 0
        PRINT("exec_num=0x%08x, NULL->NULL", ENV_VM(env)->gc_stat.exec_num);
#endif
        PRINT("NULL->NULL");
        return NULL;
    }

    YogCopyingHeader* header = (YogCopyingHeader*)ptr - 1;
    if (header->forwarding_addr != NULL) {
#if 0
        PRINT("exec_num=0x%08x, id=0x%08x, %p->(%p)", ENV_VM(env)->gc_stat.exec_num, header->id, ptr, (YogCopyingHeader*)header->forwarding_addr + 1);
#endif
        PRINT("%p->(%p)", header, header->forwarding_addr);
        return (YogCopyingHeader*)header->forwarding_addr + 1;
    }

#if 0
    GcObjectStat_increment_survive_num(&header->stat);
    increment_living_object_number(ENV_VM(env), header->stat.survive_num);
    increment_total_object_number(ENV_VM(env));
#endif

    unsigned char* dest = copying->unscanned;
    size_t size = header->size;
    memcpy(dest, header, size);

    header->forwarding_addr = dest;

    copying->unscanned += size;

#if 0
    PRINT("exec_num=0x%08x, id=0x%08x, %p->%p", ENV_VM(env)->gc_stat.exec_num, header->id, ptr, (YogCopyingHeader*)dest + 1);
#endif
    PRINT("%p->%p", header, dest);
    return (YogCopyingHeader*)dest + 1;
#undef PRINT
}

static void 
destroy_memory(void* ptr, size_t size) 
{
    memset(ptr, 0xfd, size);
}

static void 
free_heap_internal(YogCopyingHeap* heap) 
{
    destroy_memory(heap->items, heap->size);
    free(heap);
}

static void 
free_heap(YogCopying* copying) 
{
#define FREE_HEAP(heap)     do { \
    free_heap_internal((heap)); \
    (heap) = NULL; \
} while (0)
    FREE_HEAP(copying->active_heap);
    FREE_HEAP(copying->inactive_heap);
#undef FREE_HEAP
}

void 
YogCopying_iterate_objects(YogEnv* env, YogCopying* copying, void (*callback)(YogEnv*, YogCopyingHeader*))
{
    YogCopyingHeap* heap = copying->active_heap;

    unsigned char* ptr = heap->items;
    unsigned char* to = heap->free;
    while (ptr < to) {
        YogCopyingHeader* header = (YogCopyingHeader*)ptr;
        (*callback)(env, header);

        ptr += header->size;
    }
}

static void 
delete_garbage_each(YogEnv* env, YogCopyingHeader* header) 
{
    if (header->forwarding_addr == NULL) {
#if defined(DEBUG)
        DPRINTF("finalize: %p", header);
#endif
        if (header->finalizer != NULL) {
            (*header->finalizer)(env, header + 1);
        }
    }
}

void 
YogCopying_delete_garbage(YogEnv* env, YogCopying* copying) 
{
    YogCopying_iterate_objects(env, copying, delete_garbage_each);
}

void 
YogCopying_finalize(YogEnv* env, YogCopying* copying) 
{
    YogCopying_delete_garbage(env, copying);
    free_heap(copying);
}

static void 
swap_heap(YogCopyingHeap** a, YogCopyingHeap** b) 
{
    YogCopyingHeap* tmp = *a;
    *a = *b;
    *b = tmp;
}

void 
YogCopying_prepare(YogEnv* env, YogCopying* copying) 
{
    YogCopyingHeap* to_space = copying->inactive_heap;
    copying->scanned = copying->unscanned = to_space->items;
}

void
YogCopying_post_gc(YogEnv* env, YogCopying* copying)
{
    YogCopyingHeap* from_space = copying->active_heap;
    YogCopyingHeap* to_space = copying->inactive_heap;

    size_t size = from_space->free - from_space->items;
    destroy_memory(from_space->items, size);

    to_space->free = copying->unscanned;

    swap_heap(&copying->active_heap, &copying->inactive_heap);
}

void 
YogCopying_do_gc(YogEnv* env, YogCopying* copying, ObjectKeeper obj_keeper) 
{
    (*copying->root_keeper)(env, copying->root, obj_keeper, copying);

    while (copying->scanned != copying->unscanned) {
        YogCopyingHeader* header = (YogCopyingHeader*)copying->scanned;
        ChildrenKeeper keeper = header->keeper;
        if (keeper != NULL) {
            (*keeper)(env, header + 1, obj_keeper, copying);
        }

        copying->scanned += header->size;
    }

    YogCopying_delete_garbage(env, copying);
    YogCopying_post_gc(env, copying);
}

#if defined(GC_COPYING)
static void*
keep_object(YogEnv* env, void* ptr, void* heap)
{
    return YogCopying_copy(env, heap, ptr);
}

void 
YogCopying_keep_vm(YogEnv* env, YogCopying* copying) 
{
    YogVm_keep_children(env, env->vm, keep_object, copying);
}

void
YogCopying_cheney_scan(YogEnv* env, YogCopying* copying)
{
    while (copying->scanned == copying->unscanned) {
        YogCopyingHeader* header = (YogCopyingHeader*)copying->scanned;
        ChildrenKeeper keeper = header->keeper;
        if (keeper != NULL) {
            (*keeper)(env, header + 1, keep_object, copying);
        }

        copying->scanned += header->size;
    }
}

void 
YogCopying_gc(YogEnv* env, YogCopying* copying) 
{
    YogCopying_prepare(env, copying);
    YogCopying_do_gc(env, copying, keep_object);
}
#endif

void* 
YogCopying_alloc(YogEnv* env, YogCopying* copying, ChildrenKeeper keeper, Finalizer finalizer, size_t size)
{
    size_t needed_size = size + sizeof(YogCopyingHeader);
    size_t rounded_size = round_size(needed_size);
#if 0
    vm->gc_stat.total_allocated_size += rounded_size;
#endif

    YogCopyingHeap* heap = copying->active_heap;
#define REST_SIZE(heap)     ((heap)->size - ((heap)->free - (heap)->items))
    size_t rest_size = REST_SIZE(heap);
    if ((rest_size < rounded_size) || copying->stress) {
        if (!copying->stress && (heap->size < rounded_size)) {
            return NULL;
        }

#if defined(GC_COPYING)
        YogGC_perform(env);
#elif defined(GC_GENERATIONAL)
        YogGC_perform_minor(env);
        if (copying->stress) {
#if 0
            YogGenerational_oldify_all(env, gen);
#endif
            YogGC_perform_major(env);
        }
#endif
        heap = copying->active_heap;
        rest_size = REST_SIZE(heap);
        if (rest_size < rounded_size) {
            copying->err = ERR_COPYING_OUT_OF_MEMORY;
            return NULL;
        }
    }
#undef REST_SIZE

    YogCopyingHeader* header = (YogCopyingHeader*)heap->free;
#if 0
    GcObjectStat_initialize(&header->stat);
#endif
    header->keeper = keeper;
    header->finalizer = finalizer;
    header->forwarding_addr = NULL;
    header->size = rounded_size;
    static unsigned int id = 0;
    header->id = id++;
#if defined(GC_GENERATIONAL)
    header->servive_num = 0;
    header->updated = FALSE;
    header->gen = GEN_YOUNG;
#endif

    heap->free += rounded_size;

#if 0
    increment_total_object_number(vm);
#endif

    return header + 1;
}

void 
YogCopying_allocate_heap(YogEnv* env, YogCopying* copying) 
{
    if (copying->active_heap) {
        return;
    }

    size_t heap_size = copying->heap_size;
    copying->active_heap = YogCopyingHeap_new(copying, heap_size);
    copying->inactive_heap = YogCopyingHeap_new(copying, heap_size);
#if defined(DEBUG)
#   define PRINT_HEAP(text, heap)   do { \
    DPRINTF("%s: %p (%p-%p)", (text), (heap), (heap)->items, (unsigned char*)(heap)->items + (heap)->size); \
} while (0)
#else 
#   define PRINT_HEAP(text, heap)
#endif
    PRINT_HEAP("active heap", copying->active_heap);
    PRINT_HEAP("inactive heap", copying->inactive_heap);
#undef PRINT_HEAP

}

void 
YogCopying_initialize(YogEnv* env, YogCopying* copying, BOOL stress, size_t heap_size, void* root, ChildrenKeeper root_keeper) 
{
    copying->err = ERR_COPYING_NONE;
    copying->stress = stress;
    copying->heap_size = heap_size;
    copying->active_heap = NULL;
    copying->inactive_heap = NULL;
    copying->scanned = NULL;
    copying->unscanned = NULL;
    copying->root = root;
    copying->root_keeper = root_keeper;
}

#if defined(TEST_COPYING)
#define HEAP_SIZE   (1 * 1024 * 1024)

#define CREATE_TEST(name, root, root_keeper) \
    static void \
    name() \
    { \
        YogVm vm; \
        YogEnv env; \
        env.vm = &vm; \
        YogCopying_initialize(&env, &vm.gc.copying, FALSE, HEAP_SIZE, root, root_keeper); \
        \
        test_##name(&env); \
        \
        YogCopying_finalize(&env, &vm.gc.copying); \
    }

static void 
test_alloc1(YogEnv* env)
{
    YogCopying* copying = &env->vm->gc.copying;
    void* actual = YogCopying_alloc(env, copying, NULL, NULL, 0);
    YogCopyingHeap* heap = copying->active_heap;
    void* expected = (unsigned char*)heap->items + sizeof(YogCopyingHeader);
    CU_ASSERT_PTR_EQUAL(actual, expected);
}

CREATE_TEST(alloc1, NULL, NULL);

static void* gc1_ptr = NULL;

static void 
gc1_keep_children(YogEnv* env, void* ptr, ObjectKeeper keeper) 
{
    gc1_ptr = (*keeper)(env, gc1_ptr);
}

static void 
test_gc1(YogEnv* env) 
{
    YogCopying* copying = &env->vm->gc.copying;
    gc1_ptr = YogCopying_alloc(env, copying, NULL, NULL, 0);
    YogCopyingHeap* heap = copying->inactive_heap;
    void* expected = (unsigned char*)heap->items + sizeof(YogCopyingHeader);
    YogCopying_gc(env, copying);
    CU_ASSERT_PTR_EQUAL(gc1_ptr, expected);
}

CREATE_TEST(gc1, NULL, gc1_keep_children);

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

    CU_pSuite suite = CU_add_suite("copying", NULL, NULL);
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
    ADD_TEST(gc1);
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
